#include "database/world_queries.hpp"
#include "database/generated/db_tables.hpp"
#include "database/generated/db_enums.hpp"
#include "database/db_parsing_utils.hpp"
#include "core/mobile.hpp"
#include "core/logging.hpp"
#include "text/string_utils.hpp"

using DbParsingUtils::parse_pg_array;
#include <magic_enum/magic_enum.hpp>
#include <nlohmann/json.hpp>
#include <fmt/format.h>
#include <fmt/chrono.h>
#include <sstream>
#include <iomanip>
#include <random>
#include <algorithm>
#include <utility>
#include <crypt.h>
#include <openssl/rand.h>
#include <openssl/evp.h>
#include <array>
#include <cstring>

namespace WorldQueries {

// Helper functions to convert db:: enums to game enums
inline ObjectFlag to_game(db::ObjectFlag f) { return static_cast<ObjectFlag>(std::to_underlying(f)); }

/**
 * Convert database object type string to ObjectType.
 * Uses the generated db::object_type_from_db() directly since ObjectType IS db::ObjectType.
 * Logs warnings for unknown types instead of silently defaulting.
 */
static ObjectType object_type_from_db_string(std::string_view type_str,
                                              std::shared_ptr<Logger> logger,
                                              int zone_id = 0, int obj_id = 0) {
    auto result = db::object_type_from_db(type_str);
    if (!result) {
        logger->warn("Unknown object type '{}' for object ({}, {}), defaulting to Other",
                    type_str, zone_id, obj_id);
        return ObjectType::Other;
    }
    return *result;
}

// Helper function to parse PostgreSQL integer array format: {1,2,3}
static std::vector<int> parse_pg_int_array(const std::string& pg_array) {
    std::vector<int> result;
    auto str_values = parse_pg_array(pg_array);
    result.reserve(str_values.size());
    for (const auto& str : str_values) {
        try {
            result.push_back(std::stoi(str));
        } catch (const std::exception&) {
            // Skip invalid integers
        }
    }
    return result;
}

// Helper function to map database Sector enum to C++ SectorType
static SectorType sector_from_db_string(const std::string& sector_str) {
    static const std::unordered_map<std::string, SectorType> sector_map = {
        {"STRUCTURE", SectorType::Inside},
        {"CITY", SectorType::City},
        {"FIELD", SectorType::Field},
        {"FOREST", SectorType::Forest},
        {"HILLS", SectorType::Hills},
        {"MOUNTAIN", SectorType::Mountains},
        {"SHALLOWS", SectorType::Water_Swim},
        {"WATER", SectorType::Water_Noswim},
        {"UNDERWATER", SectorType::Underwater},
        {"AIR", SectorType::Flying},
        {"ROAD", SectorType::Road},
        {"GRASSLANDS", SectorType::Field},
        {"CAVE", SectorType::Underground},
        {"RUINS", SectorType::Inside},
        {"SWAMP", SectorType::Swamp},
        {"BEACH", SectorType::Beach},
        {"UNDERDARK", SectorType::Underground},
        {"ASTRALPLANE", SectorType::Astral},
        {"AIRPLANE", SectorType::Flying},
        {"FIREPLANE", SectorType::Fire},
        {"EARTHPLANE", SectorType::Underground},
        {"ETHEREALPLANE", SectorType::Spirit},
        {"AVERNUS", SectorType::Fire},
    };

    auto it = sector_map.find(sector_str);
    if (it != sector_map.end()) {
        return it->second;
    }
    return SectorType::Inside;
}

// room_flag_from_db_string REMOVED - RoomFlag replaced by baseLightLevel and Lua restrictions

// Direction conversion: use db::direction_from_db() directly
// Game's Direction enum is now unified with db::Direction via 'using' alias

Result<std::vector<std::unique_ptr<Zone>>> load_all_zones(pqxx::work& txn) {
    auto logger = Log::database();
    logger->debug("Loading all zones from database");

    try {
        // Query zones using generated column constants
        // Note: camelCase columns use aliases for reliable pqxx access
        auto result = txn.exec(
            fmt::format(R"(
                SELECT {}, {}, {}, "{}" AS reset_mode
                FROM "{}"
                ORDER BY {}
            )",
            db::Zones::ID, db::Zones::NAME, db::Zones::LIFESPAN, db::Zones::RESET_MODE,
            db::Zones::TABLE,
            db::Zones::ID
        ));

        std::vector<std::unique_ptr<Zone>> zones;
        zones.reserve(result.size());

        for (const auto& row : result) {
            int zone_id = row[db::Zones::ID.data()].as<int>();
            std::string name = row[db::Zones::NAME.data()].as<std::string>();
            int lifespan = row[db::Zones::LIFESPAN.data()].as<int>(30);

            // Create zone with composite EntityId using factory method
            // Zones use (zoneId, 1) as their ID to ensure zone 0 is valid
            // (EntityId(0, 0) is INVALID_ENTITY_ID)
            auto zone_result = Zone::create(
                EntityId(zone_id, 1),
                name,
                lifespan,
                ResetMode::Always  // Default reset mode
            );

            if (zone_result) {
                zones.push_back(std::move(*zone_result));
            }
        }

        logger->debug("Loaded {} zones from database", zones.size());
        return zones;

    } catch (const pqxx::sql_error& e) {
        logger->error("SQL error loading zones: {}", e.what());
        return std::unexpected(Error{ErrorCode::InternalError,
            fmt::format("Failed to load zones: {}", e.what())});
    }
}

Result<std::unique_ptr<Zone>> load_zone(pqxx::work& txn, int zone_id) {
    auto logger = Log::database();
    logger->debug("Loading zone {} from database", zone_id);

    try {
        // Query zone using generated column constants
        // Note: camelCase columns use aliases for reliable pqxx access
        auto result = txn.exec_params(
            fmt::format(R"(
                SELECT {}, {}, {}, "{}" AS reset_mode
                FROM "{}"
                WHERE {} = $1
            )",
            db::Zones::ID, db::Zones::NAME, db::Zones::LIFESPAN, db::Zones::RESET_MODE,
            db::Zones::TABLE,
            db::Zones::ID
        ), zone_id);

        if (result.empty()) {
            return std::unexpected(Error{ErrorCode::NotFound,
                fmt::format("Zone {} not found", zone_id)});
        }

        const auto& row = result[0];
        std::string name = row[db::Zones::NAME.data()].as<std::string>();
        int lifespan = row[db::Zones::LIFESPAN.data()].as<int>(30);

        // Zones use (zoneId, 1) to ensure zone 0 is valid
        auto zone = Zone::create(
            EntityId(zone_id, 1),
            name,
            lifespan,
            ResetMode::Always  // Default reset mode
        );

        return zone;

    } catch (const pqxx::sql_error& e) {
        logger->error("SQL error loading zone {}: {}", zone_id, e.what());
        return std::unexpected(Error{ErrorCode::InternalError,
            fmt::format("Failed to load zone {}: {}", zone_id, e.what())});
    }
}

Result<std::vector<std::unique_ptr<Room>>> load_rooms_in_zone(
    pqxx::work& txn, int zone_id) {

    auto logger = Log::database();
    logger->debug("Loading rooms for zone {} from database", zone_id);

    try {
        // Query rooms using generated column constants
        auto result = txn.exec_params(
            fmt::format(R"(
                SELECT {}, {}, {}, {}, {}, {}, {}, {}, {}, {},
                       {}, {}, {}, {}, {}, {}, {}
                FROM "{}"
                WHERE {} = $1 AND {} IS NULL
                ORDER BY {}
            )",
            db::Room::ZONE_ID, db::Room::ID, db::Room::NAME,
            db::Room::ROOM_DESCRIPTION, db::Room::SECTOR, db::Room::BASE_LIGHT_LEVEL,
            db::Room::CAPACITY, db::Room::LAYOUT_X, db::Room::LAYOUT_Y, db::Room::LAYOUT_Z,
            // Room state and restriction fields
            db::Room::IS_PEACEFUL, db::Room::ALLOWS_MAGIC, db::Room::ALLOWS_RECALL,
            db::Room::ALLOWS_SUMMON, db::Room::ALLOWS_TELEPORT, db::Room::IS_DEATH_TRAP,
            db::Room::ENTRY_RESTRICTION,
            db::Room::TABLE,
            db::Room::ZONE_ID, db::Room::DELETED_AT, db::Room::ID
        ), zone_id);

        std::vector<std::unique_ptr<Room>> rooms;
        rooms.reserve(result.size());

        for (const auto& row : result) {
            int local_id = row[db::Room::ID.data()].as<int>();
            std::string name = row[db::Room::NAME.data()].as<std::string>();
            std::string description = row[db::Room::ROOM_DESCRIPTION.data()].as<std::string>("");

            // Parse sector type from database enum
            SectorType sector = SectorType::Inside;
            if (!row[db::Room::SECTOR.data()].is_null()) {
                std::string sector_str = row[db::Room::SECTOR.data()].as<std::string>();
                sector = sector_from_db_string(sector_str);
            }

            // Create room with composite EntityId using factory method
            auto room_result = Room::create(
                EntityId(zone_id, local_id),
                name,
                sector
            );

            if (room_result) {
                auto& room = *room_result;

                // Set room description
                room->set_description(description);

                // Set zone ID (zones use local_id 1 to ensure zone 0 is valid)
                room->set_zone_id(EntityId(zone_id, 1));

                // Set base light level (lighting system)
                if (!row[db::Room::BASE_LIGHT_LEVEL.data()].is_null()) {
                    room->set_base_light_level(row[db::Room::BASE_LIGHT_LEVEL.data()].as<int>());
                }

                // Set capacity (occupancy limit)
                if (!row[db::Room::CAPACITY.data()].is_null()) {
                    room->set_capacity(row[db::Room::CAPACITY.data()].as<int>());
                }

                // Set layout coordinates (for mapping)
                std::optional<int> layout_x, layout_y, layout_z;
                if (!row[db::Room::LAYOUT_X.data()].is_null()) {
                    layout_x = row[db::Room::LAYOUT_X.data()].as<int>();
                }
                if (!row[db::Room::LAYOUT_Y.data()].is_null()) {
                    layout_y = row[db::Room::LAYOUT_Y.data()].as<int>();
                }
                if (!row[db::Room::LAYOUT_Z.data()].is_null()) {
                    layout_z = row[db::Room::LAYOUT_Z.data()].as<int>();
                }
                room->set_layout_coords(layout_x, layout_y, layout_z);

                // Set room state flags
                if (!row[db::Room::IS_PEACEFUL.data()].is_null()) {
                    room->set_peaceful(row[db::Room::IS_PEACEFUL.data()].as<bool>());
                }
                if (!row[db::Room::ALLOWS_MAGIC.data()].is_null()) {
                    room->set_allows_magic(row[db::Room::ALLOWS_MAGIC.data()].as<bool>());
                }
                if (!row[db::Room::ALLOWS_RECALL.data()].is_null()) {
                    room->set_allows_recall(row[db::Room::ALLOWS_RECALL.data()].as<bool>());
                }
                if (!row[db::Room::ALLOWS_SUMMON.data()].is_null()) {
                    room->set_allows_summon(row[db::Room::ALLOWS_SUMMON.data()].as<bool>());
                }
                if (!row[db::Room::ALLOWS_TELEPORT.data()].is_null()) {
                    room->set_allows_teleport(row[db::Room::ALLOWS_TELEPORT.data()].as<bool>());
                }
                if (!row[db::Room::IS_DEATH_TRAP.data()].is_null()) {
                    room->set_death_trap(row[db::Room::IS_DEATH_TRAP.data()].as<bool>());
                }

                // Set entry restriction Lua script
                if (!row[db::Room::ENTRY_RESTRICTION.data()].is_null()) {
                    room->set_entry_restriction(row[db::Room::ENTRY_RESTRICTION.data()].as<std::string>());
                }

                rooms.push_back(std::move(room));
            }
        }

        logger->debug("Loaded {} rooms for zone {}", rooms.size(), zone_id);
        return rooms;

    } catch (const pqxx::sql_error& e) {
        logger->error("SQL error loading rooms for zone {}: {}", zone_id, e.what());
        return std::unexpected(Error{ErrorCode::InternalError,
            fmt::format("Failed to load rooms for zone {}: {}", zone_id, e.what())});
    }
}

Result<std::unique_ptr<Room>> load_room(pqxx::work& txn, int zone_id, int room_local_id) {
    auto logger = Log::database();
    logger->debug("Loading room ({}, {}) from database", zone_id, room_local_id);

    try {
        // Query room using generated column constants
        auto result = txn.exec_params(
            fmt::format(R"(
                SELECT {}, {}, {}, {}, {}, {}, {}, {}, {}, {},
                       {}, {}, {}, {}, {}, {}, {}
                FROM "{}"
                WHERE {} = $1 AND {} = $2
            )",
            db::Room::ZONE_ID, db::Room::ID, db::Room::NAME,
            db::Room::ROOM_DESCRIPTION, db::Room::SECTOR, db::Room::BASE_LIGHT_LEVEL,
            db::Room::CAPACITY, db::Room::LAYOUT_X, db::Room::LAYOUT_Y, db::Room::LAYOUT_Z,
            // Room state and restriction fields
            db::Room::IS_PEACEFUL, db::Room::ALLOWS_MAGIC, db::Room::ALLOWS_RECALL,
            db::Room::ALLOWS_SUMMON, db::Room::ALLOWS_TELEPORT, db::Room::IS_DEATH_TRAP,
            db::Room::ENTRY_RESTRICTION,
            db::Room::TABLE,
            db::Room::ZONE_ID, db::Room::ID
        ), zone_id, room_local_id);

        if (result.empty()) {
            return std::unexpected(Error{ErrorCode::NotFound,
                fmt::format("Room ({}, {}) not found", zone_id, room_local_id)});
        }

        const auto& row = result[0];
        std::string name = row[db::Room::NAME.data()].as<std::string>();
        std::string description = row[db::Room::ROOM_DESCRIPTION.data()].as<std::string>("");

        // Parse sector type from database enum
        SectorType sector = SectorType::Inside;
        if (!row[db::Room::SECTOR.data()].is_null()) {
            std::string sector_str = row[db::Room::SECTOR.data()].as<std::string>();
            sector = sector_from_db_string(sector_str);
        }

        auto room_result = Room::create(
            EntityId(zone_id, room_local_id),
            name,
            sector
        );

        if (room_result) {
            auto& room = *room_result;

            // Set room description
            room->set_description(description);

            // Set zone ID
            room->set_zone_id(EntityId(zone_id, 1));

            // Set base light level (lighting system)
            if (!row[db::Room::BASE_LIGHT_LEVEL.data()].is_null()) {
                room->set_base_light_level(row[db::Room::BASE_LIGHT_LEVEL.data()].as<int>());
            }

            // Set capacity (occupancy limit)
            if (!row[db::Room::CAPACITY.data()].is_null()) {
                room->set_capacity(row[db::Room::CAPACITY.data()].as<int>());
            }

            // Set layout coordinates (for mapping)
            std::optional<int> layout_x, layout_y, layout_z;
            if (!row[db::Room::LAYOUT_X.data()].is_null()) {
                layout_x = row[db::Room::LAYOUT_X.data()].as<int>();
            }
            if (!row[db::Room::LAYOUT_Y.data()].is_null()) {
                layout_y = row[db::Room::LAYOUT_Y.data()].as<int>();
            }
            if (!row[db::Room::LAYOUT_Z.data()].is_null()) {
                layout_z = row[db::Room::LAYOUT_Z.data()].as<int>();
            }
            room->set_layout_coords(layout_x, layout_y, layout_z);

            // Set room state flags
            if (!row[db::Room::IS_PEACEFUL.data()].is_null()) {
                room->set_peaceful(row[db::Room::IS_PEACEFUL.data()].as<bool>());
            }
            if (!row[db::Room::ALLOWS_MAGIC.data()].is_null()) {
                room->set_allows_magic(row[db::Room::ALLOWS_MAGIC.data()].as<bool>());
            }
            if (!row[db::Room::ALLOWS_RECALL.data()].is_null()) {
                room->set_allows_recall(row[db::Room::ALLOWS_RECALL.data()].as<bool>());
            }
            if (!row[db::Room::ALLOWS_SUMMON.data()].is_null()) {
                room->set_allows_summon(row[db::Room::ALLOWS_SUMMON.data()].as<bool>());
            }
            if (!row[db::Room::ALLOWS_TELEPORT.data()].is_null()) {
                room->set_allows_teleport(row[db::Room::ALLOWS_TELEPORT.data()].as<bool>());
            }
            if (!row[db::Room::IS_DEATH_TRAP.data()].is_null()) {
                room->set_death_trap(row[db::Room::IS_DEATH_TRAP.data()].as<bool>());
            }

            // Set entry restriction Lua script
            if (!row[db::Room::ENTRY_RESTRICTION.data()].is_null()) {
                room->set_entry_restriction(row[db::Room::ENTRY_RESTRICTION.data()].as<std::string>());
            }

            return std::move(room);
        }

        return std::unexpected(Error{ErrorCode::InternalError,
            fmt::format("Failed to create room ({}, {})", zone_id, room_local_id)});

    } catch (const pqxx::sql_error& e) {
        logger->error("SQL error loading room ({}, {}): {}", zone_id, room_local_id, e.what());
        return std::unexpected(Error{ErrorCode::InternalError,
            fmt::format("Failed to load room ({}, {}): {}", zone_id, room_local_id, e.what())});
    }
}

Result<std::vector<ExitData>> load_room_exits(
    pqxx::work& txn, int zone_id, int room_local_id) {

    auto logger = Log::database();
    logger->debug("Loading exits for room ({}, {})", zone_id, room_local_id);

    try {
        // Query exits using generated column constants
        auto result = txn.exec_params(
            fmt::format(R"(
                SELECT {}, {}, {}, {}
                FROM "{}"
                WHERE {} = $1 AND {} = $2
            )",
            db::RoomExit::DIRECTION, db::RoomExit::TO_ZONE_ID,
            db::RoomExit::TO_ROOM_ID, db::RoomExit::DESCRIPTION,
            db::RoomExit::TABLE,
            db::RoomExit::ROOM_ZONE_ID, db::RoomExit::ROOM_ID
        ), zone_id, room_local_id);

        std::vector<ExitData> exits;
        exits.reserve(result.size());

        for (const auto& row : result) {
            ExitData exit;

            // Parse direction enum from database string value
            std::string dir_str = row[db::RoomExit::DIRECTION.data()].as<std::string>("");
            auto parsed_dir = RoomUtils::parse_direction(dir_str);
            if (!parsed_dir) {
                logger->warn("Invalid direction '{}' for exit in room ({}, {})", dir_str, zone_id, room_local_id);
                continue;  // Skip invalid exits
            }
            exit.direction = parsed_dir.value();

            // Handle nullable destination - skip exits with no destination
            if (row[db::RoomExit::TO_ZONE_ID.data()].is_null() || row[db::RoomExit::TO_ROOM_ID.data()].is_null()) {
                logger->debug("Exit in room ({}, {}) has null destination, skipping", zone_id, room_local_id);
                continue;
            }
            int to_zone = row[db::RoomExit::TO_ZONE_ID.data()].as<int>();
            int to_room = row[db::RoomExit::TO_ROOM_ID.data()].as<int>();
            exit.to_room = EntityId(to_zone, to_room);
            exit.description = row[db::RoomExit::DESCRIPTION.data()].as<std::string>("");

            exits.push_back(std::move(exit));
        }

        logger->debug("Loaded {} exits for room ({}, {})", exits.size(), zone_id, room_local_id);
        return exits;

    } catch (const pqxx::sql_error& e) {
        logger->error("SQL error loading exits for room ({}, {}): {}", zone_id, room_local_id, e.what());
        return std::unexpected(Error{ErrorCode::InternalError,
            fmt::format("Failed to load exits: {}", e.what())});
    }
}

// Stub implementations for mobs and objects (to be fully implemented)
Result<std::vector<std::unique_ptr<Mobile>>> load_mobs_in_zone(
    pqxx::work& txn, int zone_id) {

    auto logger = Log::database();
    logger->debug("Loading mobs for zone {} from database", zone_id);

    try {
        // Query all mobs in this zone using generated column constants
        // Note: camelCase columns use aliases for reliable pqxx access
        auto result = txn.exec_params(
            fmt::format(R"(
                SELECT {}, {}, {}, {}, {}, {},
                       {}, {}, {}, {}, {}, {}, {}, {},
                       {}, {}, {}, {}, {}, {},
                       {},
                       {}, {}, {},
                       "{}" AS life_force, {}, "{}" AS damage_type,
                       {}, {}, {},
                       {}, {},
                       {}, {}, {}, {}, {},
                       {}, {}, {}, {}, {}, {},
                       {}, {}, {},
                       {}, {},
                       "{}" AS aggression_formula
                FROM "{}"
                WHERE {} = $1
                ORDER BY {}
            )",
            // Row 1: Basic info
            db::Mobs::ZONE_ID, db::Mobs::ID, db::Mobs::NAME, db::Mobs::KEYWORDS,
            db::Mobs::ROOM_DESCRIPTION, db::Mobs::EXAMINE_DESCRIPTION,
            // Row 2: Level/dice info
            db::Mobs::LEVEL, db::Mobs::ALIGNMENT, db::Mobs::DAMAGE_DICE_NUM,
            db::Mobs::DAMAGE_DICE_SIZE, db::Mobs::DAMAGE_DICE_BONUS,
            db::Mobs::HP_DICE_NUM, db::Mobs::HP_DICE_SIZE, db::Mobs::HP_DICE_BONUS,
            // Row 3: Attributes
            db::Mobs::STRENGTH, db::Mobs::INTELLIGENCE, db::Mobs::WISDOM,
            db::Mobs::DEXTERITY, db::Mobs::CONSTITUTION, db::Mobs::CHARISMA,
            // Row 4: Currency (single wealth column in copper)
            db::Mobs::WEALTH,
            // Row 5: Race/physical
            db::Mobs::RACE, db::Mobs::GENDER, db::Mobs::SIZE,
            // Row 5b: camelCase columns with aliases
            db::Mobs::LIFE_FORCE, db::Mobs::COMPOSITION, db::Mobs::DAMAGE_TYPE,
            // Row 6: Traits/behaviors/professions (reorganized from old mobFlags)
            db::Mobs::TRAITS, db::Mobs::BEHAVIORS, db::Mobs::PROFESSIONS,
            // Row 7: Class and stance
            db::Mobs::CLASS_ID, db::Mobs::STANCE,
            // Row 8: Offensive stats
            db::Mobs::ACCURACY, db::Mobs::ATTACK_POWER, db::Mobs::SPELL_POWER,
            db::Mobs::PENETRATION_FLAT, db::Mobs::PENETRATION_PERCENT,
            // Row 9: Defensive stats
            db::Mobs::EVASION, db::Mobs::ARMOR_RATING, db::Mobs::DAMAGE_REDUCTION_PERCENT,
            db::Mobs::SOAK, db::Mobs::HARDNESS, db::Mobs::WARD_PERCENT,
            // Row 10: Other
            db::Mobs::RESISTANCES, db::Mobs::PERCEPTION, db::Mobs::CONCEALMENT,
            // Row 11: Display name components
            db::Mobs::BASE_NAME, db::Mobs::ARTICLE,
            // Row 12: Aggression formula (Lua expression)
            db::Mobs::AGGRESSION_FORMULA,
            // Table and WHERE
            db::Mobs::TABLE, db::Mobs::ZONE_ID, db::Mobs::ID
        ), zone_id);

        std::vector<std::unique_ptr<Mobile>> mobs;
        mobs.reserve(result.size());

        for (const auto& row : result) {
            int mob_zone_id = row[db::Mobs::ZONE_ID.data()].as<int>();
            int mob_id = row[db::Mobs::ID.data()].as<int>();
            EntityId mob_entity_id(mob_zone_id, mob_id);

            std::string mob_name = row[db::Mobs::NAME.data()].as<std::string>();
            int mob_level = std::max(1, row[db::Mobs::LEVEL.data()].as<int>(1));  // Clamp to minimum 1

            // Create the mobile
            auto mob_result = Mobile::create(mob_entity_id, mob_name, mob_level);
            if (!mob_result) {
                logger->error("Failed to create mob ({}, {}): {}",
                             mob_zone_id, mob_id, mob_result.error().message);
                continue;
            }

            auto mob = std::move(*mob_result);

            // Set room description (shown when mob is in room)
            if (!row[db::Mobs::ROOM_DESCRIPTION.data()].is_null()) {
                mob->set_ground(row[db::Mobs::ROOM_DESCRIPTION.data()].as<std::string>());
            }

            // Set stats from database
            Stats& mob_stats = mob->stats();

            // Base attributes
            if (!row[db::Mobs::STRENGTH.data()].is_null()) mob_stats.strength = row[db::Mobs::STRENGTH.data()].as<int>();
            if (!row[db::Mobs::INTELLIGENCE.data()].is_null()) mob_stats.intelligence = row[db::Mobs::INTELLIGENCE.data()].as<int>();
            if (!row[db::Mobs::WISDOM.data()].is_null()) mob_stats.wisdom = row[db::Mobs::WISDOM.data()].as<int>();
            if (!row[db::Mobs::DEXTERITY.data()].is_null()) mob_stats.dexterity = row[db::Mobs::DEXTERITY.data()].as<int>();
            if (!row[db::Mobs::CONSTITUTION.data()].is_null()) mob_stats.constitution = row[db::Mobs::CONSTITUTION.data()].as<int>();
            if (!row[db::Mobs::CHARISMA.data()].is_null()) mob_stats.charisma = row[db::Mobs::CHARISMA.data()].as<int>();

            // Alignment
            if (!row[db::Mobs::ALIGNMENT.data()].is_null()) mob_stats.alignment = row[db::Mobs::ALIGNMENT.data()].as<int>();

            // Offensive stats
            if (!row[db::Mobs::ACCURACY.data()].is_null()) mob_stats.accuracy = row[db::Mobs::ACCURACY.data()].as<int>();
            if (!row[db::Mobs::ATTACK_POWER.data()].is_null()) mob_stats.attack_power = row[db::Mobs::ATTACK_POWER.data()].as<int>();
            if (!row[db::Mobs::SPELL_POWER.data()].is_null()) mob_stats.spell_power = row[db::Mobs::SPELL_POWER.data()].as<int>();
            if (!row[db::Mobs::PENETRATION_FLAT.data()].is_null()) mob_stats.penetration_flat = row[db::Mobs::PENETRATION_FLAT.data()].as<int>();
            if (!row[db::Mobs::PENETRATION_PERCENT.data()].is_null()) mob_stats.penetration_percent = row[db::Mobs::PENETRATION_PERCENT.data()].as<int>();

            // Defensive stats
            if (!row[db::Mobs::EVASION.data()].is_null()) mob_stats.evasion = row[db::Mobs::EVASION.data()].as<int>();
            if (!row[db::Mobs::ARMOR_RATING.data()].is_null()) mob_stats.armor_rating = row[db::Mobs::ARMOR_RATING.data()].as<int>();
            if (!row[db::Mobs::DAMAGE_REDUCTION_PERCENT.data()].is_null()) mob_stats.damage_reduction_percent = row[db::Mobs::DAMAGE_REDUCTION_PERCENT.data()].as<int>();
            if (!row[db::Mobs::SOAK.data()].is_null()) mob_stats.soak = row[db::Mobs::SOAK.data()].as<int>();
            if (!row[db::Mobs::HARDNESS.data()].is_null()) mob_stats.hardness = row[db::Mobs::HARDNESS.data()].as<int>();
            if (!row[db::Mobs::WARD_PERCENT.data()].is_null()) mob_stats.ward_percent = row[db::Mobs::WARD_PERCENT.data()].as<int>();

            // Perception and concealment
            if (!row[db::Mobs::PERCEPTION.data()].is_null()) mob_stats.perception = row[db::Mobs::PERCEPTION.data()].as<int>();
            if (!row[db::Mobs::CONCEALMENT.data()].is_null()) mob_stats.concealment = row[db::Mobs::CONCEALMENT.data()].as<int>();

            // Currency - wealth is already stored in copper
            if (!row[db::Mobs::WEALTH.data()].is_null()) {
                mob_stats.gold = row[db::Mobs::WEALTH.data()].as<long>(0);
            }

            // Elemental resistances (from JSONB column)
            if (!row[db::Mobs::RESISTANCES.data()].is_null()) {
                try {
                    auto resistances_json = nlohmann::json::parse(row[db::Mobs::RESISTANCES.data()].as<std::string>());
                    if (resistances_json.contains("FIRE")) mob_stats.resistance_fire = resistances_json["FIRE"].get<int>();
                    if (resistances_json.contains("COLD")) mob_stats.resistance_cold = resistances_json["COLD"].get<int>();
                    if (resistances_json.contains("LIGHTNING")) mob_stats.resistance_lightning = resistances_json["LIGHTNING"].get<int>();
                    if (resistances_json.contains("ACID")) mob_stats.resistance_acid = resistances_json["ACID"].get<int>();
                    if (resistances_json.contains("POISON")) mob_stats.resistance_poison = resistances_json["POISON"].get<int>();
                } catch (const nlohmann::json::exception& e) {
                    logger->warn("Failed to parse resistances JSON for mob {}: {}", row[db::Mobs::NAME.data()].as<std::string>("unknown"), e.what());
                }
            }

            // HP dice - used to calculate HP when spawned
            int hp_num = row[db::Mobs::HP_DICE_NUM.data()].as<int>(1);
            int hp_size = row[db::Mobs::HP_DICE_SIZE.data()].as<int>(8);
            int hp_bonus = row[db::Mobs::HP_DICE_BONUS.data()].as<int>(0);
            mob->set_hp_dice(hp_num, hp_size, hp_bonus);

            // Calculate HP from dice for the prototype
            mob->calculate_hp_from_dice();

            // Set keywords (parse PostgreSQL array format)
            if (!row[db::Mobs::KEYWORDS.data()].is_null()) {
                std::string keywords_str = row[db::Mobs::KEYWORDS.data()].as<std::string>();
                auto keywords = parse_pg_array(keywords_str);
                if (!keywords.empty()) {
                    mob->set_keywords(std::span<const std::string>(keywords));
                }
            }

            // Set examine description (detailed look text)
            if (!row[db::Mobs::EXAMINE_DESCRIPTION.data()].is_null()) {
                mob->set_description(row[db::Mobs::EXAMINE_DESCRIPTION.data()].as<std::string>());
            }

            // Set mob-specific properties
            if (!row[db::Mobs::RACE.data()].is_null()) {
                mob->set_race(row[db::Mobs::RACE.data()].as<std::string>());
            }
            if (!row[db::Mobs::GENDER.data()].is_null()) {
                mob->set_gender(row[db::Mobs::GENDER.data()].as<std::string>());
            }
            if (!row[db::Mobs::SIZE.data()].is_null()) {
                mob->set_size(row[db::Mobs::SIZE.data()].as<std::string>());
            }
            // Use lowercase aliases for camelCase columns (pqxx column access quirk)
            if (!row["life_force"].is_null()) {
                mob->set_life_force(row["life_force"].as<std::string>());
            }
            if (!row[db::Mobs::COMPOSITION.data()].is_null()) {
                mob->set_composition(row[db::Mobs::COMPOSITION.data()].as<std::string>());
            }
            if (!row["damage_type"].is_null()) {
                mob->set_damage_type(row["damage_type"].as<std::string>());
            }
            if (!row[db::Mobs::STANCE.data()].is_null()) {
                mob->set_stance(row[db::Mobs::STANCE.data()].as<std::string>());
            }

            // Set damage dice for bare hand attacks
            int dam_num = row[db::Mobs::DAMAGE_DICE_NUM.data()].as<int>(1);
            int dam_size = row[db::Mobs::DAMAGE_DICE_SIZE.data()].as<int>(4);
            int dam_bonus = row[db::Mobs::DAMAGE_DICE_BONUS.data()].as<int>(0);
            mob->set_bare_hand_damage(dam_num, dam_size, dam_bonus);

            // Process traits - what mob IS (identity)
            if (!row[db::Mobs::TRAITS.data()].is_null()) {
                auto traits_str = row[db::Mobs::TRAITS.data()].as<std::string>();
                auto trait_names = parse_pg_array(traits_str);
                for (const auto& trait_name : trait_names) {
                    if (auto trait = db::mob_trait_from_db(trait_name)) {
                        mob->set_trait(*trait, true);
                    }
                }
            }

            // Process behaviors - how mob ACTS
            if (!row[db::Mobs::BEHAVIORS.data()].is_null()) {
                auto behaviors_str = row[db::Mobs::BEHAVIORS.data()].as<std::string>();
                auto behavior_names = parse_pg_array(behaviors_str);
                for (const auto& behavior_name : behavior_names) {
                    if (auto behavior = db::mob_behavior_from_db(behavior_name)) {
                        mob->set_behavior(*behavior, true);
                    }
                }
            }

            // Process professions - services mob provides
            if (!row[db::Mobs::PROFESSIONS.data()].is_null()) {
                auto professions_str = row[db::Mobs::PROFESSIONS.data()].as<std::string>();
                auto profession_names = parse_pg_array(professions_str);
                for (const auto& profession_name : profession_names) {
                    if (auto profession = db::mob_profession_from_db(profession_name)) {
                        mob->set_profession(*profession, true);
                    }
                }
            }

            // Set class_id for class-specific guildmasters
            if (!row[db::Mobs::CLASS_ID.data()].is_null()) {
                mob->set_class_id(row[db::Mobs::CLASS_ID.data()].as<int>());
            }

            // Set base_name and article for dynamic display names
            // NULL article = calculate a/an dynamically, empty string = no article, value = explicit article
            if (!row[db::Mobs::BASE_NAME.data()].is_null()) {
                mob->set_base_name(row[db::Mobs::BASE_NAME.data()].as<std::string>());
            }
            if (!row[db::Mobs::ARTICLE.data()].is_null()) {
                mob->set_article(row[db::Mobs::ARTICLE.data()].as<std::string>());
            } else {
                mob->set_article(std::nullopt);  // Calculate a/an dynamically
            }

            // Set aggression_formula (Lua expression for aggression behavior)
            // Examples: "true" (attacks all), "target.alignment <= -350" (attacks evil)
            if (!row["aggression_formula"].is_null()) {
                mob->set_aggro_condition(row["aggression_formula"].as<std::string>());
            }

            mobs.push_back(std::move(mob));
        }

        logger->debug("Loaded {} mobs for zone {}", mobs.size(), zone_id);
        return mobs;

    } catch (const pqxx::sql_error& e) {
        logger->error("SQL error loading mobs for zone {}: {}", zone_id, e.what());
        return std::unexpected(Error{ErrorCode::InternalError,
            fmt::format("Failed to load mobs: {}", e.what())});
    }
}

Result<std::unique_ptr<Mobile>> load_mob(
    pqxx::work& txn, int zone_id, int mob_local_id) {

    auto logger = Log::database();
    logger->debug("Loading mob ({}, {})", zone_id, mob_local_id);

    try {
        // Query using generated column constants
        // Note: camelCase columns use aliases for reliable pqxx access
        auto result = txn.exec_params(
            fmt::format(R"(
                SELECT {}, {}, {}, {}, {}, {},
                       {}, {}, {}, {}, {}, {}, {}, {},
                       {}, {}, {}, {}, {}, {},
                       {},
                       {}, {}, {},
                       "{}" AS life_force, {}, "{}" AS damage_type,
                       {}, {}, {},
                       {}, {},
                       {}, {}, {}, {}, {},
                       {}, {}, {}, {}, {}, {},
                       {}, {}, {},
                       {}, {},
                       "{}" AS aggression_formula
                FROM "{}"
                WHERE {} = $1 AND {} = $2
            )",
            // Row 1: Basic info
            db::Mobs::ZONE_ID, db::Mobs::ID, db::Mobs::NAME, db::Mobs::KEYWORDS,
            db::Mobs::ROOM_DESCRIPTION, db::Mobs::EXAMINE_DESCRIPTION,
            // Row 2: Level/dice info
            db::Mobs::LEVEL, db::Mobs::ALIGNMENT, db::Mobs::DAMAGE_DICE_NUM,
            db::Mobs::DAMAGE_DICE_SIZE, db::Mobs::DAMAGE_DICE_BONUS,
            db::Mobs::HP_DICE_NUM, db::Mobs::HP_DICE_SIZE, db::Mobs::HP_DICE_BONUS,
            // Row 3: Attributes
            db::Mobs::STRENGTH, db::Mobs::INTELLIGENCE, db::Mobs::WISDOM,
            db::Mobs::DEXTERITY, db::Mobs::CONSTITUTION, db::Mobs::CHARISMA,
            // Row 4: Currency (single wealth column in copper)
            db::Mobs::WEALTH,
            // Row 5: Race/physical
            db::Mobs::RACE, db::Mobs::GENDER, db::Mobs::SIZE,
            // Row 5b: camelCase columns with aliases
            db::Mobs::LIFE_FORCE, db::Mobs::COMPOSITION, db::Mobs::DAMAGE_TYPE,
            // Row 6: Traits/behaviors/professions (reorganized from old mobFlags)
            db::Mobs::TRAITS, db::Mobs::BEHAVIORS, db::Mobs::PROFESSIONS,
            // Row 7: Class and stance
            db::Mobs::CLASS_ID, db::Mobs::STANCE,
            // Row 8: Offensive stats
            db::Mobs::ACCURACY, db::Mobs::ATTACK_POWER, db::Mobs::SPELL_POWER,
            db::Mobs::PENETRATION_FLAT, db::Mobs::PENETRATION_PERCENT,
            // Row 9: Defensive stats
            db::Mobs::EVASION, db::Mobs::ARMOR_RATING, db::Mobs::DAMAGE_REDUCTION_PERCENT,
            db::Mobs::SOAK, db::Mobs::HARDNESS, db::Mobs::WARD_PERCENT,
            // Row 10: Other
            db::Mobs::RESISTANCES, db::Mobs::PERCEPTION, db::Mobs::CONCEALMENT,
            // Row 11: Display name components
            db::Mobs::BASE_NAME, db::Mobs::ARTICLE,
            // Row 12: Aggression formula (Lua expression)
            db::Mobs::AGGRESSION_FORMULA,
            // Table and WHERE
            db::Mobs::TABLE, db::Mobs::ZONE_ID, db::Mobs::ID
        ), zone_id, mob_local_id);

        if (result.empty()) {
            return std::unexpected(Error{ErrorCode::NotFound,
                fmt::format("Mob ({}, {}) not found", zone_id, mob_local_id)});
        }

        const auto& row = result[0];
        EntityId mob_entity_id(zone_id, mob_local_id);
        std::string mob_name = row[db::Mobs::NAME.data()].as<std::string>();
        int mob_level = std::max(1, row[db::Mobs::LEVEL.data()].as<int>(1));

        auto mob_result = Mobile::create(mob_entity_id, mob_name, mob_level);
        if (!mob_result) {
            return std::unexpected(mob_result.error());
        }

        auto mob = std::move(*mob_result);

        // Set room description (shown when mob is in room)
        if (!row[db::Mobs::ROOM_DESCRIPTION.data()].is_null()) {
            mob->set_ground(row[db::Mobs::ROOM_DESCRIPTION.data()].as<std::string>());
        }

        // Set stats from database
        Stats& mob_stats = mob->stats();

        // Base attributes
        if (!row[db::Mobs::STRENGTH.data()].is_null()) mob_stats.strength = row[db::Mobs::STRENGTH.data()].as<int>();
        if (!row[db::Mobs::INTELLIGENCE.data()].is_null()) mob_stats.intelligence = row[db::Mobs::INTELLIGENCE.data()].as<int>();
        if (!row[db::Mobs::WISDOM.data()].is_null()) mob_stats.wisdom = row[db::Mobs::WISDOM.data()].as<int>();
        if (!row[db::Mobs::DEXTERITY.data()].is_null()) mob_stats.dexterity = row[db::Mobs::DEXTERITY.data()].as<int>();
        if (!row[db::Mobs::CONSTITUTION.data()].is_null()) mob_stats.constitution = row[db::Mobs::CONSTITUTION.data()].as<int>();
        if (!row[db::Mobs::CHARISMA.data()].is_null()) mob_stats.charisma = row[db::Mobs::CHARISMA.data()].as<int>();

        // Alignment
        if (!row[db::Mobs::ALIGNMENT.data()].is_null()) mob_stats.alignment = row[db::Mobs::ALIGNMENT.data()].as<int>();

        // Offensive stats
        if (!row[db::Mobs::ACCURACY.data()].is_null()) mob_stats.accuracy = row[db::Mobs::ACCURACY.data()].as<int>();
        if (!row[db::Mobs::ATTACK_POWER.data()].is_null()) mob_stats.attack_power = row[db::Mobs::ATTACK_POWER.data()].as<int>();
        if (!row[db::Mobs::SPELL_POWER.data()].is_null()) mob_stats.spell_power = row[db::Mobs::SPELL_POWER.data()].as<int>();
        if (!row[db::Mobs::PENETRATION_FLAT.data()].is_null()) mob_stats.penetration_flat = row[db::Mobs::PENETRATION_FLAT.data()].as<int>();
        if (!row[db::Mobs::PENETRATION_PERCENT.data()].is_null()) mob_stats.penetration_percent = row[db::Mobs::PENETRATION_PERCENT.data()].as<int>();

        // Defensive stats
        if (!row[db::Mobs::EVASION.data()].is_null()) mob_stats.evasion = row[db::Mobs::EVASION.data()].as<int>();
        if (!row[db::Mobs::ARMOR_RATING.data()].is_null()) mob_stats.armor_rating = row[db::Mobs::ARMOR_RATING.data()].as<int>();
        if (!row[db::Mobs::DAMAGE_REDUCTION_PERCENT.data()].is_null()) mob_stats.damage_reduction_percent = row[db::Mobs::DAMAGE_REDUCTION_PERCENT.data()].as<int>();
        if (!row[db::Mobs::SOAK.data()].is_null()) mob_stats.soak = row[db::Mobs::SOAK.data()].as<int>();
        if (!row[db::Mobs::HARDNESS.data()].is_null()) mob_stats.hardness = row[db::Mobs::HARDNESS.data()].as<int>();
        if (!row[db::Mobs::WARD_PERCENT.data()].is_null()) mob_stats.ward_percent = row[db::Mobs::WARD_PERCENT.data()].as<int>();

        // Perception and concealment
        if (!row[db::Mobs::PERCEPTION.data()].is_null()) mob_stats.perception = row[db::Mobs::PERCEPTION.data()].as<int>();
        if (!row[db::Mobs::CONCEALMENT.data()].is_null()) mob_stats.concealment = row[db::Mobs::CONCEALMENT.data()].as<int>();

        // Currency - wealth is already stored in copper
        if (!row[db::Mobs::WEALTH.data()].is_null()) {
            mob_stats.gold = row[db::Mobs::WEALTH.data()].as<long>(0);
        }

        // Elemental resistances (from JSONB column)
        if (!row[db::Mobs::RESISTANCES.data()].is_null()) {
            try {
                auto resistances_json = nlohmann::json::parse(row[db::Mobs::RESISTANCES.data()].as<std::string>());
                if (resistances_json.contains("FIRE")) mob_stats.resistance_fire = resistances_json["FIRE"].get<int>();
                if (resistances_json.contains("COLD")) mob_stats.resistance_cold = resistances_json["COLD"].get<int>();
                if (resistances_json.contains("LIGHTNING")) mob_stats.resistance_lightning = resistances_json["LIGHTNING"].get<int>();
                if (resistances_json.contains("ACID")) mob_stats.resistance_acid = resistances_json["ACID"].get<int>();
                if (resistances_json.contains("POISON")) mob_stats.resistance_poison = resistances_json["POISON"].get<int>();
            } catch (const nlohmann::json::exception& e) {
                logger->warn("Failed to parse resistances JSON for mob {}: {}", mob_name, e.what());
            }
        }

        // HP dice - used to calculate HP when spawned
        int hp_num = row[db::Mobs::HP_DICE_NUM.data()].as<int>(1);
        int hp_size = row[db::Mobs::HP_DICE_SIZE.data()].as<int>(8);
        int hp_bonus = row[db::Mobs::HP_DICE_BONUS.data()].as<int>(0);
        mob->set_hp_dice(hp_num, hp_size, hp_bonus);

        // Calculate HP from dice for the prototype
        mob->calculate_hp_from_dice();

        // Set keywords (parse PostgreSQL array format)
        if (!row[db::Mobs::KEYWORDS.data()].is_null()) {
            std::string keywords_str = row[db::Mobs::KEYWORDS.data()].as<std::string>();
            auto keywords = parse_pg_array(keywords_str);
            if (!keywords.empty()) {
                mob->set_keywords(std::span<const std::string>(keywords));
            }
        }

        // Set examine description (detailed look text)
        if (!row[db::Mobs::EXAMINE_DESCRIPTION.data()].is_null()) {
            mob->set_description(row[db::Mobs::EXAMINE_DESCRIPTION.data()].as<std::string>());
        }

        // Set mob-specific properties
        if (!row[db::Mobs::RACE.data()].is_null()) {
            mob->set_race(row[db::Mobs::RACE.data()].as<std::string>());
        }
        if (!row[db::Mobs::GENDER.data()].is_null()) {
            mob->set_gender(row[db::Mobs::GENDER.data()].as<std::string>());
        }
        if (!row[db::Mobs::SIZE.data()].is_null()) {
            mob->set_size(row[db::Mobs::SIZE.data()].as<std::string>());
        }
        // Use lowercase aliases for camelCase columns (pqxx column access quirk)
        if (!row["life_force"].is_null()) {
            mob->set_life_force(row["life_force"].as<std::string>());
        }
        if (!row[db::Mobs::COMPOSITION.data()].is_null()) {
            mob->set_composition(row[db::Mobs::COMPOSITION.data()].as<std::string>());
        }
        if (!row["damage_type"].is_null()) {
            mob->set_damage_type(row["damage_type"].as<std::string>());
        }
        if (!row[db::Mobs::STANCE.data()].is_null()) {
            mob->set_stance(row[db::Mobs::STANCE.data()].as<std::string>());
        }

        // Set damage dice for bare hand attacks
        int dam_num = row[db::Mobs::DAMAGE_DICE_NUM.data()].as<int>(1);
        int dam_size = row[db::Mobs::DAMAGE_DICE_SIZE.data()].as<int>(4);
        int dam_bonus = row[db::Mobs::DAMAGE_DICE_BONUS.data()].as<int>(0);
        mob->set_bare_hand_damage(dam_num, dam_size, dam_bonus);

        // Process traits - what mob IS (identity)
        if (!row[db::Mobs::TRAITS.data()].is_null()) {
            auto traits_str = row[db::Mobs::TRAITS.data()].as<std::string>();
            auto trait_names = parse_pg_array(traits_str);
            for (const auto& trait_name : trait_names) {
                if (auto trait = db::mob_trait_from_db(trait_name)) {
                    mob->set_trait(*trait, true);
                }
            }
        }

        // Process behaviors - how mob ACTS
        if (!row[db::Mobs::BEHAVIORS.data()].is_null()) {
            auto behaviors_str = row[db::Mobs::BEHAVIORS.data()].as<std::string>();
            auto behavior_names = parse_pg_array(behaviors_str);
            for (const auto& behavior_name : behavior_names) {
                if (auto behavior = db::mob_behavior_from_db(behavior_name)) {
                    mob->set_behavior(*behavior, true);
                }
            }
        }

        // Process professions - services mob provides
        if (!row[db::Mobs::PROFESSIONS.data()].is_null()) {
            auto professions_str = row[db::Mobs::PROFESSIONS.data()].as<std::string>();
            auto profession_names = parse_pg_array(professions_str);
            for (const auto& profession_name : profession_names) {
                if (auto profession = db::mob_profession_from_db(profession_name)) {
                    mob->set_profession(*profession, true);
                }
            }
        }

        // Set class_id for class-specific guildmasters
        if (!row[db::Mobs::CLASS_ID.data()].is_null()) {
            mob->set_class_id(row[db::Mobs::CLASS_ID.data()].as<int>());
        }

        // Set base_name and article for dynamic display names
        // NULL article = calculate a/an dynamically, empty string = no article, value = explicit article
        if (!row[db::Mobs::BASE_NAME.data()].is_null()) {
            mob->set_base_name(row[db::Mobs::BASE_NAME.data()].as<std::string>());
        }
        if (!row[db::Mobs::ARTICLE.data()].is_null()) {
            mob->set_article(row[db::Mobs::ARTICLE.data()].as<std::string>());
        } else {
            mob->set_article(std::nullopt);  // Calculate a/an dynamically
        }

        // Set aggression_formula (Lua expression for aggression behavior)
        if (!row["aggression_formula"].is_null()) {
            mob->set_aggro_condition(row["aggression_formula"].as<std::string>());
        }

        logger->debug("Loaded mob '{}' ({}, {})", mob_name, zone_id, mob_local_id);
        return mob;

    } catch (const pqxx::sql_error& e) {
        logger->error("SQL error loading mob ({}, {}): {}", zone_id, mob_local_id, e.what());
        return std::unexpected(Error{ErrorCode::InternalError,
            fmt::format("Failed to load mob: {}", e.what())});
    }
}

Result<std::vector<std::unique_ptr<Object>>> load_objects_in_zone(
    pqxx::work& txn, int zone_id) {

    auto logger = Log::database();
    logger->debug("Loading objects for zone {} from database", zone_id);

    try {
        // Query all objects in this zone using generated column constants
        // Note: camelCase columns use aliases for reliable pqxx access
        auto result = txn.exec_params(
            fmt::format(R"(
                SELECT {}, {}, {}, {}, {}, {}, {},
                       {}, {}, {}, {}, {}, {},
                       "{}" AS wear_flags,
                       {}, {}
                FROM "{}"
                WHERE {} = $1
                ORDER BY {}
            )",
            db::Objects::ZONE_ID, db::Objects::ID, db::Objects::NAME,
            db::Objects::KEYWORDS, db::Objects::TYPE, db::Objects::ROOM_DESCRIPTION,
            db::Objects::EXAMINE_DESCRIPTION, db::Objects::WEIGHT, db::Objects::COST,
            db::Objects::LEVEL, db::Objects::TIMER, db::Objects::VALUES,
            db::Objects::FLAGS, db::Objects::WEAR_FLAGS,
            db::Objects::BASE_NAME, db::Objects::ARTICLE,
            db::Objects::TABLE, db::Objects::ZONE_ID, db::Objects::ID
        ), zone_id);

        std::vector<std::unique_ptr<Object>> objects;
        objects.reserve(result.size());

        for (const auto& row : result) {
            int obj_zone_id = row[db::Objects::ZONE_ID.data()].as<int>();
            int obj_id = row[db::Objects::ID.data()].as<int>();
            EntityId obj_entity_id(obj_zone_id, obj_id);

            std::string obj_name = row[db::Objects::NAME.data()].as<std::string>();

            // Parse object type using data-driven helper
            std::string type_str = row[db::Objects::TYPE.data()].as<std::string>();
            ObjectType obj_type = object_type_from_db_string(type_str, logger, obj_zone_id, obj_id);

            // Create the object
            auto obj_result = Object::create(obj_entity_id, obj_name, obj_type);
            if (!obj_result) {
                logger->error("Failed to create object ({}, {}): {}",
                             obj_zone_id, obj_id, obj_result.error().message);
                continue;
            }

            auto obj = std::move(*obj_result);

            // Set weight
            if (!row[db::Objects::WEIGHT.data()].is_null()) {
                obj->set_weight(static_cast<int>(row[db::Objects::WEIGHT.data()].as<double>()));
            }

            // Set cost/value
            if (!row[db::Objects::COST.data()].is_null()) {
                obj->set_value(row[db::Objects::COST.data()].as<int>());
            }

            // Set level
            if (!row[db::Objects::LEVEL.data()].is_null()) {
                obj->set_level(row[db::Objects::LEVEL.data()].as<int>());
            }

            // Set timer
            if (!row[db::Objects::TIMER.data()].is_null()) {
                obj->set_timer(row[db::Objects::TIMER.data()].as<int>());
            }

            // Set room/ground description (shown when object is on the ground)
            if (!row[db::Objects::ROOM_DESCRIPTION.data()].is_null()) {
                obj->set_ground(row[db::Objects::ROOM_DESCRIPTION.data()].as<std::string>());
            }

            // Set examine description (detailed description when looking at object)
            if (!row[db::Objects::EXAMINE_DESCRIPTION.data()].is_null()) {
                obj->set_examine_description(row[db::Objects::EXAMINE_DESCRIPTION.data()].as<std::string>());
            }

            // Set keywords (parse PostgreSQL array format)
            if (!row[db::Objects::KEYWORDS.data()].is_null()) {
                std::string keywords_str = row[db::Objects::KEYWORDS.data()].as<std::string>();
                auto keywords = parse_pg_array(keywords_str);
                if (!keywords.empty()) {
                    obj->set_keywords(std::span<const std::string>(keywords));
                }
            }

            // Set base_name for dynamic article handling
            if (!row[db::Objects::BASE_NAME.data()].is_null()) {
                obj->set_base_name(row[db::Objects::BASE_NAME.data()].as<std::string>());
            }

            // Set article for dynamic name building
            // NULL = use a/an based on first letter, "" = no article, "the"/"some" = specific article
            if (!row[db::Objects::ARTICLE.data()].is_null()) {
                obj->set_article(row[db::Objects::ARTICLE.data()].as<std::string>());
            }
            // If article column is NULL, leave article_ as nullopt (means calculate a/an at runtime)

            // Set equip slot and can_take from wearFlags (using lowercase alias for pqxx access)
            // Default to not takeable - only objects with TAKE flag can be picked up
            obj->set_can_take(false);
            if (!row["wear_flags"].is_null()) {
                std::string wear_flags_str = row["wear_flags"].as<std::string>();
                auto wear_flags = parse_pg_array(wear_flags_str);
                for (const auto& flag : wear_flags) {
                    // Check for TAKE flag - allows object to be picked up
                    if (flag == "TAKE") {
                        obj->set_can_take(true);
                        continue;
                    }

                    // Try to parse this flag as an equip slot
                    auto slot_opt = ObjectUtils::parse_equip_slot(flag);
                    if (slot_opt && *slot_opt != EquipSlot::None) {
                        obj->set_equip_slot(*slot_opt);
                        break;  // Use the first valid wear position found
                    }
                }
            }

            // Parse values JSON for object-specific data
            if (!row[db::Objects::VALUES.data()].is_null()) {
                try {
                    std::string values_str = row[db::Objects::VALUES.data()].as<std::string>();
                    auto values_json = nlohmann::json::parse(values_str);

                    // Parse liquid info for drink containers
                    if (obj_type == ObjectType::Drinkcontainer && values_json.is_object()) {
                        LiquidInfo liquid;
                        if (values_json.contains("Liquid")) {
                            liquid.liquid_type = values_json["Liquid"].get<std::string>();
                        }
                        if (values_json.contains("Capacity")) {
                            liquid.capacity = values_json["Capacity"].get<int>();
                        }
                        if (values_json.contains("Remaining")) {
                            liquid.remaining = values_json["Remaining"].get<int>();
                        }
                        // Load effects array if present
                        if (values_json.contains("Effects") && values_json["Effects"].is_array()) {
                            for (const auto& effect_id : values_json["Effects"]) {
                                if (effect_id.is_number_integer()) {
                                    liquid.effects.push_back(effect_id.get<int>());
                                }
                            }
                        }
                        obj->set_liquid_info(liquid);
                    }

                    // Parse light info for light sources
                    if (obj_type == ObjectType::Light && values_json.is_object()) {
                        LightInfo light;
                        // Remaining is the fuel duration in hours
                        if (values_json.contains("Remaining")) {
                            light.duration = values_json["Remaining"].get<int>();
                        }
                        // Handle typo in database key "Is_Lit:" as well as "Is_Lit"
                        if (values_json.contains("Is_Lit:")) {
                            light.lit = values_json["Is_Lit:"].get<bool>();
                        } else if (values_json.contains("Is_Lit")) {
                            light.lit = values_json["Is_Lit"].get<bool>();
                        }
                        // Brightness defaults to 1 if not specified
                        if (values_json.contains("Brightness")) {
                            light.brightness = values_json["Brightness"].get<int>();
                        }
                        // Permanent lights cannot be extinguished
                        if (values_json.contains("Permanent")) {
                            light.permanent = values_json["Permanent"].get<bool>();
                            if (light.permanent) {
                                light.lit = true;  // Permanent lights are always lit
                            }
                        }
                        obj->set_light_info(light);
                    }

                    // Parse board number for message boards
                    if (obj_type == ObjectType::Board) {
                        logger->trace("Loading board object ({}, {})", obj_zone_id, obj_id);
                        if (values_json.is_object() && values_json.contains("Pages")) {
                            int board_num = values_json["Pages"].get<int>();
                            obj->set_board_number(board_num);
                            logger->trace("Set board number {} for board ({}, {})",
                                        board_num, obj_zone_id, obj_id);
                        }
                    }
                } catch (const nlohmann::json::exception& e) {
                    logger->warn("Failed to parse values JSON for object ({}, {}): {}",
                                obj_zone_id, obj_id, e.what());
                }
            }

            objects.push_back(std::move(obj));
        }

        // Load object effects from junction table (joined with Effect to get effect names)
        auto effects_result = txn.exec_params(
            fmt::format(R"(
                SELECT oe.{}, e.{}
                FROM "{}" oe
                JOIN "{}" e ON oe.{} = e.{}
                WHERE oe.{} = $1
                ORDER BY oe.{}, oe.{}
            )",
            db::ObjectEffects::OBJECT_ID, db::Effect::NAME,
            db::ObjectEffects::TABLE, db::Effect::TABLE,
            db::ObjectEffects::EFFECT_ID, db::Effect::ID,
            db::ObjectEffects::OBJECT_ZONE_ID,
            db::ObjectEffects::OBJECT_ID, db::ObjectEffects::ID
        ), zone_id);

        // Build a map from object_id to effect names
        std::unordered_map<int, std::vector<std::string>> effects_by_obj_id;
        for (const auto& row : effects_result) {
            int obj_id = row[db::ObjectEffects::OBJECT_ID.data()].as<int>();
            std::string effect_name = row[db::Effect::NAME.data()].as<std::string>();
            effects_by_obj_id[obj_id].push_back(effect_name);
        }

        // Apply effects to objects
        for (auto& obj : objects) {
            int obj_local_id = obj->id().local_id();
            auto it = effects_by_obj_id.find(obj_local_id);
            if (it != effects_by_obj_id.end()) {
                for (const auto& effect_name : it->second) {
                    // Convert effect name to EffectFlag enum using magic_enum
                    auto effect_opt = magic_enum::enum_cast<EffectFlag>(effect_name);
                    if (effect_opt) {
                        obj->set_effect(*effect_opt, true);
                    } else {
                        logger->warn("Unknown effect '{}' for object ({}, {})",
                                    effect_name, zone_id, obj_local_id);
                    }
                }
            }
        }

        // Load extra descriptions for all objects in this zone
        auto extras_result = txn.exec_params(
            fmt::format(R"(
                SELECT {}, {}, {}
                FROM "{}"
                WHERE {} = $1
                ORDER BY {}, {}
            )",
            db::ObjectExtraDescriptions::OBJECT_ID,
            db::ObjectExtraDescriptions::KEYWORDS,
            db::ObjectExtraDescriptions::DESCRIPTION,
            db::ObjectExtraDescriptions::TABLE,
            db::ObjectExtraDescriptions::OBJECT_ZONE_ID,
            db::ObjectExtraDescriptions::OBJECT_ID,
            db::ObjectExtraDescriptions::ID
        ), zone_id);

        // Build a map from object_id to extra descriptions
        std::unordered_map<int, std::vector<ExtraDescription>> extras_by_obj_id;
        for (const auto& row : extras_result) {
            int obj_id = row[db::ObjectExtraDescriptions::OBJECT_ID.data()].as<int>();
            std::string keywords_str = row[db::ObjectExtraDescriptions::KEYWORDS.data()].as<std::string>();
            std::string description = row[db::ObjectExtraDescriptions::DESCRIPTION.data()].as<std::string>();

            ExtraDescription extra;
            extra.keywords = parse_pg_array(keywords_str);
            extra.description = description;
            extras_by_obj_id[obj_id].push_back(std::move(extra));
        }

        // Assign extra descriptions to objects
        for (auto& obj : objects) {
            int obj_local_id = obj->id().local_id();
            auto it = extras_by_obj_id.find(obj_local_id);
            if (it != extras_by_obj_id.end()) {
                for (const auto& extra : it->second) {
                    obj->add_extra_description(extra);
                }
            }
        }

        logger->debug("Loaded {} objects with {} extra descriptions for zone {}",
                     objects.size(), extras_result.size(), zone_id);
        return objects;

    } catch (const pqxx::sql_error& e) {
        logger->error("SQL error loading objects for zone {}: {}", zone_id, e.what());
        return std::unexpected(Error{ErrorCode::InternalError,
            fmt::format("Failed to load objects: {}", e.what())});
    }
}

Result<std::unique_ptr<Object>> load_object(
    pqxx::work& txn, int zone_id, int object_local_id) {

    auto logger = Log::database();
    logger->debug("Loading object ({}, {})", zone_id, object_local_id);

    try {
        auto result = txn.exec_params(
            // Note: camelCase columns use aliases for reliable pqxx access
            fmt::format(R"(
                SELECT {}, {}, {}, {}, {}, {}, {},
                       {}, {}, {}, {}, {}, {},
                       "{}" AS wear_flags,
                       {}, {}
                FROM "{}"
                WHERE {} = $1 AND {} = $2
            )",
            db::Objects::ZONE_ID, db::Objects::ID, db::Objects::NAME,
            db::Objects::KEYWORDS, db::Objects::TYPE, db::Objects::ROOM_DESCRIPTION,
            db::Objects::EXAMINE_DESCRIPTION, db::Objects::WEIGHT, db::Objects::COST,
            db::Objects::LEVEL, db::Objects::TIMER, db::Objects::VALUES,
            db::Objects::FLAGS, db::Objects::WEAR_FLAGS,
            db::Objects::BASE_NAME, db::Objects::ARTICLE,
            db::Objects::TABLE, db::Objects::ZONE_ID, db::Objects::ID
        ), zone_id, object_local_id);

        if (result.empty()) {
            return std::unexpected(Error{ErrorCode::NotFound,
                fmt::format("Object ({}, {}) not found", zone_id, object_local_id)});
        }

        const auto& row = result[0];
        EntityId obj_entity_id(zone_id, object_local_id);
        std::string obj_name = row[db::Objects::NAME.data()].as<std::string>();

        // Parse object type using data-driven helper
        std::string type_str = row[db::Objects::TYPE.data()].is_null()
            ? "OTHER"
            : row[db::Objects::TYPE.data()].as<std::string>();
        ObjectType obj_type = object_type_from_db_string(type_str, logger, zone_id, object_local_id);

        auto obj_result = Object::create(obj_entity_id, obj_name, obj_type);
        if (!obj_result) {
            return std::unexpected(obj_result.error());
        }

        auto obj = std::move(*obj_result);

        // Set descriptions
        if (!row[db::Objects::ROOM_DESCRIPTION.data()].is_null()) {
            obj->set_description(row[db::Objects::ROOM_DESCRIPTION.data()].as<std::string>());
        }
        // examine_description stored in short_desc for detailed look
        if (!row[db::Objects::EXAMINE_DESCRIPTION.data()].is_null()) {
            obj->set_short_description(row[db::Objects::EXAMINE_DESCRIPTION.data()].as<std::string>());
        }

        // Set basic properties
        if (!row[db::Objects::WEIGHT.data()].is_null()) {
            obj->set_weight(row[db::Objects::WEIGHT.data()].as<int>());
        }
        if (!row[db::Objects::COST.data()].is_null()) {
            obj->set_value(row[db::Objects::COST.data()].as<int>());
        }
        if (!row[db::Objects::LEVEL.data()].is_null()) {
            obj->set_level(row[db::Objects::LEVEL.data()].as<int>());
        }
        if (!row[db::Objects::TIMER.data()].is_null()) {
            obj->set_timer(row[db::Objects::TIMER.data()].as<int>());
        }

        // Set keywords (parse PostgreSQL array format)
        if (!row[db::Objects::KEYWORDS.data()].is_null()) {
            std::string keywords_str = row[db::Objects::KEYWORDS.data()].as<std::string>();
            auto keywords = parse_pg_array(keywords_str);
            if (!keywords.empty()) {
                obj->set_keywords(std::span<const std::string>(keywords));
            }
        }

        // Set base_name for dynamic article handling
        if (!row[db::Objects::BASE_NAME.data()].is_null()) {
            obj->set_base_name(row[db::Objects::BASE_NAME.data()].as<std::string>());
        }

        // Set article for dynamic name building
        // NULL = use a/an based on first letter, "" = no article, "the"/"some" = specific article
        if (!row[db::Objects::ARTICLE.data()].is_null()) {
            obj->set_article(row[db::Objects::ARTICLE.data()].as<std::string>());
        }
        // If article column is NULL, leave article_ as nullopt (means calculate a/an at runtime)

        // Parse object values JSON - can be JSON object or array
        // Note: Modern Object class uses a single value, not legacy values[0-3]
        if (!row[db::Objects::VALUES.data()].is_null()) {
            try {
                std::string values_str = row[db::Objects::VALUES.data()].as<std::string>();
                auto values_json = nlohmann::json::parse(values_str);
                if (values_json.is_array() && !values_json.empty() && values_json[0].is_number()) {
                    // First value often represents primary object-specific data
                    // For now, we store it in object value if cost wasn't set
                    if (row[db::Objects::COST.data()].is_null()) {
                        obj->set_value(values_json[0].get<int>());
                    }
                } else if (values_json.is_object()) {
                    // Parse liquid info for drink containers
                    if (obj_type == ObjectType::Drinkcontainer) {
                        LiquidInfo liquid;
                        if (values_json.contains("Liquid")) {
                            liquid.liquid_type = values_json["Liquid"].get<std::string>();
                        }
                        if (values_json.contains("Capacity")) {
                            liquid.capacity = values_json["Capacity"].get<int>();
                        }
                        if (values_json.contains("Remaining")) {
                            liquid.remaining = values_json["Remaining"].get<int>();
                        }
                        // Load effects array if present
                        if (values_json.contains("Effects") && values_json["Effects"].is_array()) {
                            for (const auto& effect_id : values_json["Effects"]) {
                                if (effect_id.is_number_integer()) {
                                    liquid.effects.push_back(effect_id.get<int>());
                                }
                            }
                        }
                        obj->set_liquid_info(liquid);
                    }

                    // Parse light info for light sources
                    if (obj_type == ObjectType::Light) {
                        LightInfo light;
                        // Remaining is the fuel duration in hours
                        if (values_json.contains("Remaining")) {
                            light.duration = values_json["Remaining"].get<int>();
                        }
                        // Handle typo in database key "Is_Lit:" as well as "Is_Lit"
                        if (values_json.contains("Is_Lit:")) {
                            light.lit = values_json["Is_Lit:"].get<bool>();
                        } else if (values_json.contains("Is_Lit")) {
                            light.lit = values_json["Is_Lit"].get<bool>();
                        }
                        // Brightness defaults to 1 if not specified
                        if (values_json.contains("Brightness")) {
                            light.brightness = values_json["Brightness"].get<int>();
                        }
                        // Permanent lights cannot be extinguished
                        if (values_json.contains("Permanent")) {
                            light.permanent = values_json["Permanent"].get<bool>();
                            if (light.permanent) {
                                light.lit = true;  // Permanent lights are always lit
                            }
                        }
                        obj->set_light_info(light);
                    }

                    // Parse board number for message boards
                    if (obj_type == ObjectType::Board) {
                        if (values_json.contains("Pages")) {
                            obj->set_board_number(values_json["Pages"].get<int>());
                        }
                    }
                }
            } catch (const nlohmann::json::exception& e) {
                logger->warn("Failed to parse values for object ({}, {}): {}",
                            zone_id, object_local_id, e.what());
            }
        }

        // Parse object flags (PostgreSQL array format)
        // Use generated db::object_flag_from_db for SCREAMING_SNAKE_CASE -> PascalCase conversion
        if (!row[db::Objects::FLAGS.data()].is_null()) {
            std::string flags_str = row[db::Objects::FLAGS.data()].as<std::string>();
            auto flag_names = parse_pg_array(flags_str);
            for (const auto& flag_name : flag_names) {
                if (auto flag = db::object_flag_from_db(flag_name)) {
                    obj->set_flag(to_game(*flag));
                } else {
                    logger->warn("Object ({}, {}): unknown flag '{}'", zone_id, object_local_id, flag_name);
                }
            }
        }

        // Load object effects from junction table
        auto effects_result = txn.exec_params(
            fmt::format(R"(
                SELECT e.{}
                FROM "{}" oe
                JOIN "{}" e ON oe.{} = e.{}
                WHERE oe.{} = $1 AND oe.{} = $2
            )",
            db::Effect::NAME,
            db::ObjectEffects::TABLE, db::Effect::TABLE,
            db::ObjectEffects::EFFECT_ID, db::Effect::ID,
            db::ObjectEffects::OBJECT_ZONE_ID, db::ObjectEffects::OBJECT_ID
        ), zone_id, object_local_id);

        for (const auto& effect_row : effects_result) {
            std::string effect_name = effect_row[db::Effect::NAME.data()].as<std::string>();
            auto effect_opt = magic_enum::enum_cast<EffectFlag>(effect_name);
            if (effect_opt) {
                obj->set_effect(*effect_opt, true);
            } else {
                logger->warn("Unknown effect '{}' for object ({}, {})",
                            effect_name, zone_id, object_local_id);
            }
        }

        // Set equip slot and can_take from wearFlags (using lowercase alias for pqxx access)
        // Default to not takeable - only objects with TAKE flag can be picked up
        obj->set_can_take(false);
        if (!row["wear_flags"].is_null()) {
            std::string wear_flags_str = row["wear_flags"].as<std::string>();
            auto wear_flags = parse_pg_array(wear_flags_str);
            for (const auto& flag : wear_flags) {
                // Check for TAKE flag - allows object to be picked up
                if (flag == "TAKE") {
                    obj->set_can_take(true);
                    continue;
                }

                // Try to parse this flag as an equip slot
                auto slot_opt = ObjectUtils::parse_equip_slot(flag);
                if (slot_opt && *slot_opt != EquipSlot::None) {
                    obj->set_equip_slot(*slot_opt);
                    break;  // Use the first valid wear position found
                }
            }
        }

        logger->debug("Loaded object '{}' ({}, {})", obj_name, zone_id, object_local_id);
        return obj;

    } catch (const pqxx::sql_error& e) {
        logger->error("SQL error loading object ({}, {}): {}", zone_id, object_local_id, e.what());
        return std::unexpected(Error{ErrorCode::InternalError,
            fmt::format("Failed to load object: {}", e.what())});
    }
}

Result<std::vector<ExitData>> load_all_exits_in_zone(pqxx::work& txn, int zone_id) {
    auto logger = Log::database();
    logger->debug("Loading all exits for zone {} from database", zone_id);

    try {
        auto result = txn.exec_params(R"(
            SELECT
                re.room_zone_id, re.room_id,
                re.direction, re.description, re.keywords, re.key,
                re.to_zone_id, re.to_room_id, re.flags
            FROM "RoomExit" re
            WHERE re.room_zone_id = $1
            ORDER BY re.room_zone_id, re.room_id, re.direction
        )", zone_id);

        std::vector<ExitData> exits;
        exits.reserve(result.size());

        for (const auto& row : result) {
            ExitData exit;

            // Parse direction using generated converter
            std::string dir_str = row["direction"].as<std::string>();
            auto dir = db::direction_from_db(dir_str);
            if (!dir) {
                logger->warn("Unknown direction '{}' in exit for room ({}, {})",
                            dir_str, zone_id, row["room_id"].as<int>());
                continue;
            }
            exit.direction = *dir;

            // Parse destination room (composite key)
            if (!row["to_zone_id"].is_null() && !row["to_room_id"].is_null()) {
                int to_zone = row["to_zone_id"].as<int>();
                int to_room = row["to_room_id"].as<int>();
                exit.to_room = EntityId(to_zone, to_room);
            } else {
                exit.to_room = INVALID_ENTITY_ID;
            }

            // Description
            if (!row["description"].is_null()) {
                exit.description = row["description"].as<std::string>();
            }

            // Keywords (PostgreSQL array)
            if (!row["keywords"].is_null()) {
                exit.keywords = parse_pg_array(row["keywords"].as<std::string>());
            }

            // Key (parse as object reference if present)
            // Note: Key in DB is a string, but we store it as EntityId for the key object
            // For now, we'll leave it as INVALID_ENTITY_ID and handle key lookup elsewhere

            // Flags (PostgreSQL array)
            if (!row["flags"].is_null()) {
                exit.flags = parse_pg_array(row["flags"].as<std::string>());
            }

            exits.push_back(std::move(exit));
        }

        logger->debug("Loaded {} exits for zone {}", exits.size(), zone_id);
        return exits;

    } catch (const pqxx::sql_error& e) {
        logger->error("SQL error loading exits for zone {}: {}", zone_id, e.what());
        return std::unexpected(Error{ErrorCode::InternalError,
            fmt::format("Failed to load exits: {}", e.what())});
    }
}

Result<std::vector<MobResetData>> load_mob_resets_in_zone(pqxx::work& txn, int zone_id) {
    auto logger = Log::database();
    logger->debug("Loading mob resets for zone {} from database", zone_id);

    try {
        auto result = txn.exec_params(R"(
            SELECT
                id, mob_zone_id, mob_id, room_zone_id, room_id,
                max_instances, probability, comment
            FROM "MobResets"
            WHERE zone_id = $1
            ORDER BY id
        )", zone_id);

        std::vector<MobResetData> resets;
        resets.reserve(result.size());

        for (const auto& row : result) {
            MobResetData reset;
            reset.id = row["id"].as<int>();
            reset.mob_id = EntityId(
                row["mob_zone_id"].as<int>(),
                row["mob_id"].as<int>()
            );
            reset.room_id = EntityId(
                row["room_zone_id"].as<int>(),
                row["room_id"].as<int>()
            );
            reset.max_instances = row["max_instances"].as<int>(1);
            reset.probability = row["probability"].as<float>(1.0f);
            if (!row["comment"].is_null()) {
                reset.comment = row["comment"].as<std::string>();
            }
            resets.push_back(std::move(reset));
        }

        logger->debug("Loaded {} mob resets for zone {}", resets.size(), zone_id);
        return resets;

    } catch (const pqxx::sql_error& e) {
        logger->error("SQL error loading mob resets for zone {}: {}", zone_id, e.what());
        return std::unexpected(Error{ErrorCode::InternalError,
            fmt::format("Failed to load mob resets: {}", e.what())});
    }
}

Result<std::vector<MobEquipmentData>> load_mob_equipment_for_reset(pqxx::work& txn, int reset_id) {
    auto logger = Log::database();
    logger->debug("Loading equipment for mob reset {}", reset_id);

    try {
        auto result = txn.exec_params(R"(
            SELECT
                reset_id, object_zone_id, object_id,
                wear_location, max_instances, probability
            FROM "MobResetEquipment"
            WHERE reset_id = $1
            ORDER BY id
        )", reset_id);

        std::vector<MobEquipmentData> equipment;
        equipment.reserve(result.size());

        for (const auto& row : result) {
            MobEquipmentData equip;
            equip.reset_id = row["reset_id"].as<int>();
            equip.object_id = EntityId(
                row["object_zone_id"].as<int>(),
                row["object_id"].as<int>()
            );
            if (!row["wear_location"].is_null()) {
                equip.wear_location = row["wear_location"].as<std::string>();
            }
            equip.max_instances = row["max_instances"].as<int>(1);
            equip.probability = row["probability"].as<float>(1.0f);
            equipment.push_back(std::move(equip));
        }

        return equipment;

    } catch (const pqxx::sql_error& e) {
        logger->error("SQL error loading equipment for reset {}: {}", reset_id, e.what());
        return std::unexpected(Error{ErrorCode::InternalError,
            fmt::format("Failed to load mob equipment: {}", e.what())});
    }
}

Result<std::vector<MobEquipmentData>> load_all_mob_equipment_in_zone(pqxx::work& txn, int zone_id) {
    auto logger = Log::database();
    logger->debug("Loading all mob equipment for zone {}", zone_id);

    try {
        auto result = txn.exec_params(R"(
            SELECT
                mre.reset_id, mre.object_zone_id, mre.object_id,
                mre.wear_location, mre.max_instances, mre.probability
            FROM "MobResetEquipment" mre
            JOIN "MobResets" mr ON mr.id = mre.reset_id
            WHERE mr.zone_id = $1
            ORDER BY mre.reset_id, mre.id
        )", zone_id);

        std::vector<MobEquipmentData> equipment;
        equipment.reserve(result.size());

        for (const auto& row : result) {
            MobEquipmentData equip;
            equip.reset_id = row["reset_id"].as<int>();
            equip.object_id = EntityId(
                row["object_zone_id"].as<int>(),
                row["object_id"].as<int>()
            );
            if (!row["wear_location"].is_null()) {
                equip.wear_location = row["wear_location"].as<std::string>();
            }
            equip.max_instances = row["max_instances"].as<int>(1);
            equip.probability = row["probability"].as<float>(1.0f);
            equipment.push_back(std::move(equip));
        }

        logger->debug("Loaded {} mob equipment items for zone {}", equipment.size(), zone_id);
        return equipment;

    } catch (const pqxx::sql_error& e) {
        logger->error("SQL error loading mob equipment for zone {}: {}", zone_id, e.what());
        return std::unexpected(Error{ErrorCode::InternalError,
            fmt::format("Failed to load mob equipment: {}", e.what())});
    }
}

namespace {
    // Helper to build content tree from flat results (used by load_object_resets_in_zone)
    std::vector<ObjectResetContentData> build_content_tree(
        const std::unordered_map<int, std::vector<int>>& children_map,
        const std::unordered_map<int, ObjectResetContentData>& content_map,
        int parent_id  // 0 means root (direct child of reset)
    ) {
        std::vector<ObjectResetContentData> result;

        auto it = children_map.find(parent_id);
        if (it == children_map.end()) {
            return result;
        }

        for (int child_id : it->second) {
            auto content_it = content_map.find(child_id);
            if (content_it != content_map.end()) {
                ObjectResetContentData content = content_it->second;
                // Recursively build nested contents
                content.contents = build_content_tree(children_map, content_map, child_id);
                result.push_back(std::move(content));
            }
        }

        return result;
    }
}

Result<std::vector<ObjectResetData>> load_object_resets_in_zone(pqxx::work& txn, int zone_id) {
    auto logger = Log::database();
    logger->debug("Loading object resets for zone {} from database", zone_id);

    try {
        // Load base object resets (root objects that spawn in rooms)
        auto reset_result = txn.exec_params(R"(
            SELECT
                id, object_zone_id, object_id,
                room_zone_id, room_id,
                max_instances, probability, comment
            FROM "ObjectResets"
            WHERE zone_id = $1
            ORDER BY id
        )", zone_id);

        // Load all contents for this zone's resets in one query
        auto content_result = txn.exec_params(R"(
            SELECT
                orc.id, orc.reset_id, orc.parent_content_id,
                orc.object_zone_id, orc.object_id,
                orc.quantity, orc.comment
            FROM "ObjectResetContents" orc
            JOIN "ObjectResets" r ON r.id = orc.reset_id
            WHERE r.zone_id = $1
            ORDER BY orc.id
        )", zone_id);

        // Build lookup maps for nested content construction
        // Maps: reset_id -> { parent_id -> [child_ids] } and content_id -> ContentData
        std::unordered_map<int, std::unordered_map<int, std::vector<int>>> reset_children;
        std::unordered_map<int, std::unordered_map<int, ObjectResetContentData>> reset_contents;

        for (const auto& row : content_result) {
            int content_id = row["id"].as<int>();
            int reset_id = row["reset_id"].as<int>();
            int parent_id = row["parent_content_id"].is_null() ? 0 : row["parent_content_id"].as<int>();

            ObjectResetContentData content;
            content.id = content_id;
            content.object_id = EntityId(
                row["object_zone_id"].as<int>(),
                row["object_id"].as<int>()
            );
            content.quantity = row["quantity"].as<int>(1);
            if (!row["comment"].is_null()) {
                content.comment = row["comment"].as<std::string>();
            }

            reset_children[reset_id][parent_id].push_back(content_id);
            reset_contents[reset_id][content_id] = std::move(content);
        }

        // Build reset data with hierarchical contents
        std::vector<ObjectResetData> resets;
        resets.reserve(reset_result.size());

        for (const auto& row : reset_result) {
            ObjectResetData reset;
            reset.id = row["id"].as<int>();
            reset.object_id = EntityId(
                row["object_zone_id"].as<int>(),
                row["object_id"].as<int>()
            );
            reset.room_id = EntityId(
                row["room_zone_id"].as<int>(),
                row["room_id"].as<int>()
            );
            reset.max_instances = row["max_instances"].as<int>(1);
            reset.probability = row["probability"].as<float>(1.0f);
            if (!row["comment"].is_null()) {
                reset.comment = row["comment"].as<std::string>();
            }

            // Build hierarchical contents for this reset
            auto children_it = reset_children.find(reset.id);
            auto contents_it = reset_contents.find(reset.id);
            if (children_it != reset_children.end() && contents_it != reset_contents.end()) {
                reset.contents = build_content_tree(
                    children_it->second,
                    contents_it->second,
                    0  // Start with root contents (parent_id = 0/null)
                );
            }

            resets.push_back(std::move(reset));
        }

        logger->debug("Loaded {} object resets for zone {} ({} total content items)",
            resets.size(), zone_id, content_result.size());
        return resets;

    } catch (const pqxx::sql_error& e) {
        logger->error("SQL error loading object resets for zone {}: {}", zone_id, e.what());
        return std::unexpected(Error{ErrorCode::InternalError,
            fmt::format("Failed to load object resets: {}", e.what())});
    }
}

// ============================================================================
// Ability/Skill System Queries
// ============================================================================

namespace {
    AbilityType parse_ability_type(const std::string& type_str) {
        if (type_str == "SKILL") return AbilityType::Skill;
        if (type_str == "CHANT") return AbilityType::Chant;
        if (type_str == "SONG") return AbilityType::Song;
        return AbilityType::Spell;  // Default
    }

    // Parse Position enum string from database to integer
    // Maps to C++ Position enum values (lower = can use in worse condition)
    int parse_position(const std::string& pos_str) {
        if (pos_str == "PRONE") return 5;      // Sleeping/Resting level
        if (pos_str == "SITTING") return 7;    // Sitting
        if (pos_str == "KNEELING") return 7;   // Treat as Sitting
        if (pos_str == "STANDING") return 9;   // Standing
        if (pos_str == "FLYING") return 9;     // Standing (need to stand to fly)
        return 9;  // Default to Standing
    }
}

Result<std::vector<AbilityData>> load_all_abilities(pqxx::work& txn) {
    auto logger = Log::database();
    logger->debug("Loading all abilities from database");

    try {
        auto result = txn.exec(R"(
            SELECT
                a.id, a.name, a.plain_name, a.description,
                a."abilityType" AS ability_type, a."minPosition" AS min_position, a.violent,
                a.cast_time_rounds, a.cooldown_ms,
                a.is_area, a.is_toggle, a.combat_ok, a.in_combat_only,
                a.sphere, a.damage_type,
                a.pages, a.memorization_time, a.quest_only, a.humanoid_only,
                COALESCE('CORPSE' = ANY(t.valid_targets), false) AS targets_corpse
            FROM "Ability" a
            LEFT JOIN "AbilityTargeting" t ON t.ability_id = a.id
            ORDER BY a.id
        )");

        std::vector<AbilityData> abilities;
        abilities.reserve(result.size());

        for (const auto& row : result) {
            AbilityData ability;
            ability.id = row["id"].as<int>();
            ability.name = row["name"].as<std::string>("");
            ability.plain_name = row["plain_name"].as<std::string>("");
            ability.description = row["description"].as<std::string>("");
            ability.type = parse_ability_type(row["ability_type"].as<std::string>("SPELL"));
            ability.min_position = parse_position(row["min_position"].as<std::string>("STANDING"));
            ability.violent = row["violent"].as<bool>(false);
            ability.cast_time_rounds = row["cast_time_rounds"].as<int>(1);
            ability.cooldown_ms = row["cooldown_ms"].as<int>(0);
            ability.is_area = row["is_area"].as<bool>(false);
            ability.is_toggle = row["is_toggle"].as<bool>(false);
            ability.combat_ok = row["combat_ok"].as<bool>(true);
            ability.in_combat_only = row["in_combat_only"].as<bool>(false);
            ability.sphere = row["sphere"].is_null() ? "" : row["sphere"].as<std::string>();
            ability.damage_type = row["damage_type"].is_null() ? "" : row["damage_type"].as<std::string>();
            ability.pages = row["pages"].is_null() ? 0 : row["pages"].as<int>();
            ability.memorization_time = row["memorization_time"].as<int>(0);
            ability.quest_only = row["quest_only"].as<bool>(false);
            ability.humanoid_only = row["humanoid_only"].as<bool>(false);
            ability.targets_corpse = row["targets_corpse"].as<bool>(false);
            abilities.push_back(std::move(ability));
        }

        logger->debug("Loaded {} abilities from database", abilities.size());
        return abilities;

    } catch (const pqxx::sql_error& e) {
        logger->error("SQL error loading abilities: {}", e.what());
        return std::unexpected(Error{ErrorCode::InternalError,
            fmt::format("Failed to load abilities: {}", e.what())});
    } catch (const std::exception& e) {
        logger->error("Exception loading abilities: {}", e.what());
        return std::unexpected(Error{ErrorCode::InternalError,
            fmt::format("Failed to load abilities: {}", e.what())});
    }
}

Result<AbilityData> load_ability(pqxx::work& txn, int ability_id) {
    auto logger = Log::database();
    logger->debug("Loading ability {}", ability_id);

    try {
        auto result = txn.exec_params(R"(
            SELECT
                a.id, a.name, a.plain_name, a.description,
                a."abilityType" AS ability_type, a."minPosition" AS min_position, a.violent,
                a.cast_time_rounds, a.cooldown_ms,
                a.is_area, a.is_toggle, a.combat_ok, a.in_combat_only,
                a.sphere, a.damage_type,
                a.pages, a.memorization_time, a.quest_only, a.humanoid_only,
                COALESCE('CORPSE' = ANY(t.valid_targets), false) AS targets_corpse
            FROM "Ability" a
            LEFT JOIN "AbilityTargeting" t ON t.ability_id = a.id
            WHERE a.id = $1
        )", ability_id);

        if (result.empty()) {
            return std::unexpected(Error{ErrorCode::NotFound,
                fmt::format("Ability {} not found", ability_id)});
        }

        const auto& row = result[0];
        AbilityData ability;
        ability.id = row["id"].as<int>();
        ability.name = row["name"].as<std::string>("");
        ability.plain_name = row["plain_name"].as<std::string>("");
        ability.description = row["description"].as<std::string>("");
        ability.type = parse_ability_type(row["ability_type"].as<std::string>("SPELL"));
        ability.min_position = parse_position(row["min_position"].as<std::string>("STANDING"));
        ability.violent = row["violent"].as<bool>(false);
        ability.cast_time_rounds = row["cast_time_rounds"].as<int>(1);
        ability.cooldown_ms = row["cooldown_ms"].as<int>(0);
        ability.is_area = row["is_area"].as<bool>(false);
        ability.is_toggle = row["is_toggle"].as<bool>(false);
        ability.combat_ok = row["combat_ok"].as<bool>(true);
        ability.in_combat_only = row["in_combat_only"].as<bool>(false);
        ability.sphere = row["sphere"].is_null() ? "" : row["sphere"].as<std::string>();
        ability.damage_type = row["damage_type"].is_null() ? "" : row["damage_type"].as<std::string>();
        ability.pages = row["pages"].is_null() ? 0 : row["pages"].as<int>();
        ability.memorization_time = row["memorization_time"].as<int>(0);
        ability.quest_only = row["quest_only"].as<bool>(false);
        ability.humanoid_only = row["humanoid_only"].as<bool>(false);
        ability.targets_corpse = row["targets_corpse"].as<bool>(false);

        return ability;

    } catch (const pqxx::sql_error& e) {
        logger->error("SQL error loading ability {}: {}", ability_id, e.what());
        return std::unexpected(Error{ErrorCode::InternalError,
            fmt::format("Failed to load ability: {}", e.what())});
    }
}

Result<AbilityData> load_ability_by_name(pqxx::work& txn, const std::string& name) {
    auto logger = Log::database();
    logger->debug("Loading ability by name: {}", name);

    try {
        // Case-insensitive search on plain_name
        auto result = txn.exec_params(R"(
            SELECT
                id, name, plain_name, description,
                "abilityType" AS ability_type, "minPosition" AS min_position, violent,
                cast_time_rounds, cooldown_ms,
                is_area, sphere, damage_type,
                pages, memorization_time, quest_only, humanoid_only
            FROM "Ability"
            WHERE UPPER(plain_name) = UPPER($1)
        )", name);

        if (result.empty()) {
            return std::unexpected(Error{ErrorCode::NotFound,
                fmt::format("Ability '{}' not found", name)});
        }

        const auto& row = result[0];
        AbilityData ability;
        ability.id = row["id"].as<int>();
        ability.name = row["name"].as<std::string>("");
        ability.plain_name = row["plain_name"].as<std::string>("");
        ability.description = row["description"].as<std::string>("");
        ability.type = parse_ability_type(row["ability_type"].as<std::string>("SPELL"));
        ability.min_position = parse_position(row["min_position"].as<std::string>("STANDING"));
        ability.violent = row["violent"].as<bool>(false);
        ability.cast_time_rounds = row["cast_time_rounds"].as<int>(1);
        ability.cooldown_ms = row["cooldown_ms"].as<int>(0);
        ability.is_area = row["is_area"].as<bool>(false);
        ability.sphere = row["sphere"].is_null() ? "" : row["sphere"].as<std::string>();
        ability.damage_type = row["damage_type"].is_null() ? "" : row["damage_type"].as<std::string>();
        ability.pages = row["pages"].is_null() ? 0 : row["pages"].as<int>();
        ability.memorization_time = row["memorization_time"].as<int>(0);
        ability.quest_only = row["quest_only"].as<bool>(false);
        ability.humanoid_only = row["humanoid_only"].as<bool>(false);

        return ability;

    } catch (const pqxx::sql_error& e) {
        logger->error("SQL error loading ability '{}': {}", name, e.what());
        return std::unexpected(Error{ErrorCode::InternalError,
            fmt::format("Failed to load ability: {}", e.what())});
    }
}

Result<std::vector<ClassAbilityData>> load_class_abilities(pqxx::work& txn, int class_id) {
    auto logger = Log::database();
    logger->debug("Loading abilities for class {}", class_id);

    try {
        // Load both skills (from ClassSkills) and spells (from ClassAbilities)
        // Skills have min_level, spells have circle (get min_level from ClassAbilityCircles)
        auto result = txn.exec_params(R"(
            SELECT ability_id, class_id, min_level, 0 AS circle
            FROM "ClassSkills"
            WHERE class_id = $1
            UNION ALL
            SELECT ca.ability_id, ca.class_id, COALESCE(cac.min_level, 1) AS min_level, ca.circle
            FROM "ClassAbilities" ca
            LEFT JOIN "ClassAbilityCircles" cac ON cac.class_id = ca.class_id AND cac.circle = ca.circle
            WHERE ca.class_id = $1
            ORDER BY min_level, ability_id
        )", class_id);

        std::vector<ClassAbilityData> abilities;
        abilities.reserve(result.size());

        for (const auto& row : result) {
            ClassAbilityData ability;
            ability.ability_id = row["ability_id"].as<int>();
            ability.class_id = row["class_id"].as<int>();
            ability.min_level = row["min_level"].as<int>();
            ability.circle = row["circle"].as<int>(0);
            abilities.push_back(ability);
        }

        logger->debug("Loaded {} class abilities for class {}", abilities.size(), class_id);
        return abilities;

    } catch (const pqxx::sql_error& e) {
        logger->error("SQL error loading class abilities for {}: {}", class_id, e.what());
        return std::unexpected(Error{ErrorCode::InternalError,
            fmt::format("Failed to load class abilities: {}", e.what())});
    }
}

Result<std::vector<CharacterAbilityData>> load_character_abilities(
    pqxx::work& txn, const std::string& character_id) {
    auto logger = Log::database();
    logger->debug("Loading abilities for character {}", character_id);

    try {
        auto result = txn.exec_params(R"(
            SELECT character_id, ability_id, known, proficiency, last_used
            FROM "CharacterAbilities"
            WHERE character_id = $1
            ORDER BY ability_id
        )", character_id);

        std::vector<CharacterAbilityData> abilities;
        abilities.reserve(result.size());

        for (const auto& row : result) {
            CharacterAbilityData ability;
            ability.character_id = row["character_id"].as<std::string>();
            ability.ability_id = row["ability_id"].as<int>();
            ability.known = row["known"].as<bool>(false);
            ability.proficiency = row["proficiency"].as<int>(0);
            // last_used is optional
            if (!row["last_used"].is_null()) {
                // Parse timestamp - pqxx provides it as string, convert to time_point
                // For now we'll leave it as nullopt - can add proper parsing later
                ability.last_used = std::nullopt;
            }
            abilities.push_back(ability);
        }

        logger->debug("Loaded {} abilities for character {}", abilities.size(), character_id);
        return abilities;

    } catch (const pqxx::sql_error& e) {
        logger->error("SQL error loading character abilities for {}: {}", character_id, e.what());
        return std::unexpected(Error{ErrorCode::InternalError,
            fmt::format("Failed to load character abilities: {}", e.what())});
    }
}

Result<void> save_character_ability(
    pqxx::work& txn,
    const std::string& character_id,
    int ability_id,
    bool known,
    int proficiency) {
    auto logger = Log::database();
    logger->debug("Saving ability {} for character {} (known={}, prof={})",
                 ability_id, character_id, known, proficiency);

    try {
        // Upsert - insert or update on conflict
        txn.exec_params(R"(
            INSERT INTO "CharacterAbilities" (character_id, ability_id, known, proficiency)
            VALUES ($1, $2, $3, $4)
            ON CONFLICT (character_id, ability_id)
            DO UPDATE SET known = $3, proficiency = $4
        )", character_id, ability_id, known, proficiency);

        logger->debug("Saved ability {} for character {}", ability_id, character_id);
        return {};

    } catch (const pqxx::sql_error& e) {
        logger->error("SQL error saving character ability: {}", e.what());
        return std::unexpected(Error{ErrorCode::InternalError,
            fmt::format("Failed to save character ability: {}", e.what())});
    }
}

Result<std::vector<CharacterAbilityWithMetadata>> load_character_abilities_with_metadata(
    pqxx::work& txn, const std::string& character_id) {
    auto logger = Log::database();
    logger->debug("Loading abilities with metadata for character {}", character_id);

    try {
        // Single efficient query joining CharacterAbilities with Ability and ClassAbilities for circle
        auto result = txn.exec_params(R"(
            SELECT
                ca.ability_id,
                a.name,
                a.plain_name,
                a.description,
                ca.known,
                ca.proficiency,
                a."abilityType" AS ability_type,
                a.violent,
                COALESCE(cla.circle, 0) AS circle,
                a.sphere
            FROM "CharacterAbilities" ca
            JOIN "Ability" a ON a.id = ca.ability_id
            JOIN "Characters" c ON c.id = ca.character_id
            LEFT JOIN "ClassAbilities" cla ON cla.class_id = c.class_id AND cla.ability_id = ca.ability_id
            WHERE ca.character_id = $1
            ORDER BY a."abilityType", a.name
        )", character_id);

        std::vector<CharacterAbilityWithMetadata> abilities;
        abilities.reserve(result.size());

        for (const auto& row : result) {
            CharacterAbilityWithMetadata ability;
            ability.ability_id = row["ability_id"].as<int>();
            ability.name = row["name"].as<std::string>("");
            ability.plain_name = row["plain_name"].as<std::string>("");
            ability.description = row["description"].as<std::string>("");
            ability.known = row["known"].as<bool>(false);
            ability.proficiency = row["proficiency"].as<int>(0);
            ability.violent = row["violent"].as<bool>(false);

            // Convert ability type string to enum using shared helper
            ability.type = parse_ability_type(row["ability_type"].as<std::string>("SPELL"));

            // Get spell circle from ClassAbilities join
            ability.circle = row["circle"].as<int>(0);

            // Get sphere from Ability table (may be NULL)
            if (!row["sphere"].is_null()) {
                ability.sphere = row["sphere"].as<std::string>();
            }

            abilities.push_back(ability);
        }

        logger->debug("Loaded {} abilities with metadata for character {}",
                     abilities.size(), character_id);
        return abilities;

    } catch (const pqxx::sql_error& e) {
        logger->error("SQL error loading character abilities with metadata for {}: {}",
                     character_id, e.what());
        return std::unexpected(Error{ErrorCode::InternalError,
            fmt::format("Failed to load character abilities: {}", e.what())});
    }
}

// =============================================================================
// Effect System Queries
// =============================================================================

Result<std::vector<EffectData>> load_all_effects(pqxx::work& txn) {
    auto logger = Log::database();
    logger->debug("Loading all effects from database");

    try {
        auto result = txn.exec(R"(
            SELECT
                id, name, description,
                "effectType" AS effect_type,
                default_params::text AS default_params
            FROM "Effect"
            ORDER BY id
        )");

        std::vector<EffectData> effects;
        effects.reserve(result.size());

        for (const auto& row : result) {
            EffectData effect;
            effect.id = row["id"].as<int>();
            effect.name = row["name"].as<std::string>("");
            effect.description = row["description"].as<std::string>("");
            effect.effect_type = row["effect_type"].as<std::string>("");
            effect.default_params = row["default_params"].is_null() ? "{}" : row["default_params"].as<std::string>();
            effects.push_back(effect);
        }

        logger->debug("Loaded {} effects from database", effects.size());
        return effects;

    } catch (const pqxx::sql_error& e) {
        logger->error("SQL error loading effects: {}", e.what());
        return std::unexpected(Error{ErrorCode::InternalError,
            fmt::format("Failed to load effects: {}", e.what())});
    }
}

Result<EffectData> load_effect(pqxx::work& txn, int effect_id) {
    auto logger = Log::database();
    logger->debug("Loading effect by ID: {}", effect_id);

    try {
        auto result = txn.exec_params(R"(
            SELECT
                id, name, description,
                "effectType" AS effect_type,
                default_params::text AS default_params
            FROM "Effect"
            WHERE id = $1
        )", effect_id);

        if (result.empty()) {
            return std::unexpected(Error{ErrorCode::NotFound,
                fmt::format("Effect with ID {} not found", effect_id)});
        }

        const auto& row = result[0];
        EffectData effect;
        effect.id = row["id"].as<int>();
        effect.name = row["name"].as<std::string>("");
        effect.description = row["description"].as<std::string>("");
        effect.effect_type = row["effect_type"].as<std::string>("");
        effect.default_params = row["default_params"].is_null() ? "{}" : row["default_params"].as<std::string>();

        return effect;

    } catch (const pqxx::sql_error& e) {
        logger->error("SQL error loading effect {}: {}", effect_id, e.what());
        return std::unexpected(Error{ErrorCode::InternalError,
            fmt::format("Failed to load effect: {}", e.what())});
    }
}

Result<EffectData> load_effect_by_name(pqxx::work& txn, const std::string& name) {
    auto logger = Log::database();
    logger->debug("Loading effect by name: {}", name);

    try {
        auto result = txn.exec_params(R"(
            SELECT
                id, name, description,
                "effectType" AS effect_type,
                default_params::text AS default_params
            FROM "Effect"
            WHERE LOWER(name) = LOWER($1)
        )", name);

        if (result.empty()) {
            return std::unexpected(Error{ErrorCode::NotFound,
                fmt::format("Effect '{}' not found", name)});
        }

        const auto& row = result[0];
        EffectData effect;
        effect.id = row["id"].as<int>();
        effect.name = row["name"].as<std::string>("");
        effect.description = row["description"].as<std::string>("");
        effect.effect_type = row["effect_type"].as<std::string>("");
        effect.default_params = row["default_params"].is_null() ? "{}" : row["default_params"].as<std::string>();

        return effect;

    } catch (const pqxx::sql_error& e) {
        logger->error("SQL error loading effect '{}': {}", name, e.what());
        return std::unexpected(Error{ErrorCode::InternalError,
            fmt::format("Failed to load effect: {}", e.what())});
    }
}

// =============================================================================
// Character Active Effects Persistence
// =============================================================================

Result<std::vector<CharacterEffectData>> load_character_effects(
    pqxx::work& txn, const std::string& character_id) {
    auto logger = Log::database();
    logger->debug("Loading character effects for: {}", character_id);

    try {
        auto result = txn.exec_params(R"(
            SELECT
                ce.id,
                ce.character_id,
                ce.effect_id,
                ce.duration,
                ce.strength,
                ce.modifier_data::text AS modifier_data,
                ce.source_type,
                ce.source_id,
                ce.applied_at,
                ce.expires_at,
                e.name AS effect_name,
                e."effectType" AS effect_type
            FROM "CharacterEffects" ce
            JOIN "Effect" e ON ce.effect_id = e.id
            WHERE ce.character_id = $1
            ORDER BY ce.applied_at
        )", character_id);

        std::vector<CharacterEffectData> effects;
        effects.reserve(result.size());

        for (const auto& row : result) {
            CharacterEffectData effect;
            effect.id = row["id"].as<int>();
            effect.character_id = row["character_id"].as<std::string>();
            effect.effect_id = row["effect_id"].as<int>();

            if (!row["duration"].is_null()) {
                effect.duration_seconds = row["duration"].as<int>();
            }

            effect.strength = row["strength"].as<int>(1);
            effect.modifier_data = row["modifier_data"].is_null() ? "{}" : row["modifier_data"].as<std::string>();
            effect.source_type = row["source_type"].is_null() ? "" : row["source_type"].as<std::string>();

            if (!row["source_id"].is_null()) {
                effect.source_id = row["source_id"].as<int>();
            }

            // Parse timestamps
            auto applied_str = row["applied_at"].as<std::string>();
            // PostgreSQL timestamp format: "2024-01-15 10:30:00"
            std::tm tm = {};
            std::istringstream ss(applied_str);
            ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
            effect.applied_at = std::chrono::system_clock::from_time_t(std::mktime(&tm));

            if (!row["expires_at"].is_null()) {
                auto expires_str = row["expires_at"].as<std::string>();
                std::tm tm_exp = {};
                std::istringstream ss_exp(expires_str);
                ss_exp >> std::get_time(&tm_exp, "%Y-%m-%d %H:%M:%S");
                effect.expires_at = std::chrono::system_clock::from_time_t(std::mktime(&tm_exp));
            }

            effect.effect_name = row["effect_name"].as<std::string>("");
            effect.effect_type = row["effect_type"].as<std::string>("");

            effects.push_back(std::move(effect));
        }

        logger->debug("Loaded {} effects for character {}", effects.size(), character_id);
        return effects;

    } catch (const pqxx::sql_error& e) {
        logger->error("SQL error loading character effects: {}", e.what());
        return std::unexpected(Error{ErrorCode::InternalError,
            fmt::format("Failed to load character effects: {}", e.what())});
    }
}

Result<void> save_character_effects(
    pqxx::work& txn,
    const std::string& character_id,
    const std::vector<CharacterEffectData>& effects) {
    auto logger = Log::database();
    logger->debug("Saving {} effects for character {}", effects.size(), character_id);

    try {
        // First delete all existing effects for this character
        txn.exec_params(R"(
            DELETE FROM "CharacterEffects"
            WHERE character_id = $1
        )", character_id);

        // Insert new effects
        for (const auto& effect : effects) {
            // Format timestamps for PostgreSQL
            auto format_time = [](const std::chrono::system_clock::time_point& tp) -> std::string {
                auto time_t_val = std::chrono::system_clock::to_time_t(tp);
                std::tm tm = *std::gmtime(&time_t_val);
                std::ostringstream ss;
                ss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
                return ss.str();
            };

            std::string applied_at_str = format_time(effect.applied_at);

            if (effect.expires_at.has_value()) {
                std::string expires_at_str = format_time(*effect.expires_at);

                if (effect.duration_seconds.has_value() && effect.source_id.has_value()) {
                    txn.exec_params(R"(
                        INSERT INTO "CharacterEffects"
                        (character_id, effect_id, duration, strength, modifier_data,
                         source_type, source_id, applied_at, expires_at)
                        VALUES ($1, $2, $3, $4, $5::jsonb, $6, $7, $8::timestamp, $9::timestamp)
                    )", character_id, effect.effect_id, *effect.duration_seconds,
                        effect.strength, effect.modifier_data, effect.source_type,
                        *effect.source_id, applied_at_str, expires_at_str);
                } else if (effect.duration_seconds.has_value()) {
                    txn.exec_params(R"(
                        INSERT INTO "CharacterEffects"
                        (character_id, effect_id, duration, strength, modifier_data,
                         source_type, applied_at, expires_at)
                        VALUES ($1, $2, $3, $4, $5::jsonb, $6, $7::timestamp, $8::timestamp)
                    )", character_id, effect.effect_id, *effect.duration_seconds,
                        effect.strength, effect.modifier_data, effect.source_type,
                        applied_at_str, expires_at_str);
                } else if (effect.source_id.has_value()) {
                    txn.exec_params(R"(
                        INSERT INTO "CharacterEffects"
                        (character_id, effect_id, strength, modifier_data,
                         source_type, source_id, applied_at, expires_at)
                        VALUES ($1, $2, $3, $4::jsonb, $5, $6, $7::timestamp, $8::timestamp)
                    )", character_id, effect.effect_id, effect.strength,
                        effect.modifier_data, effect.source_type,
                        *effect.source_id, applied_at_str, expires_at_str);
                } else {
                    txn.exec_params(R"(
                        INSERT INTO "CharacterEffects"
                        (character_id, effect_id, strength, modifier_data,
                         source_type, applied_at, expires_at)
                        VALUES ($1, $2, $3, $4::jsonb, $5, $6::timestamp, $7::timestamp)
                    )", character_id, effect.effect_id, effect.strength,
                        effect.modifier_data, effect.source_type,
                        applied_at_str, expires_at_str);
                }
            } else {
                // No expires_at
                if (effect.duration_seconds.has_value() && effect.source_id.has_value()) {
                    txn.exec_params(R"(
                        INSERT INTO "CharacterEffects"
                        (character_id, effect_id, duration, strength, modifier_data,
                         source_type, source_id, applied_at)
                        VALUES ($1, $2, $3, $4, $5::jsonb, $6, $7, $8::timestamp)
                    )", character_id, effect.effect_id, *effect.duration_seconds,
                        effect.strength, effect.modifier_data, effect.source_type,
                        *effect.source_id, applied_at_str);
                } else if (effect.duration_seconds.has_value()) {
                    txn.exec_params(R"(
                        INSERT INTO "CharacterEffects"
                        (character_id, effect_id, duration, strength, modifier_data,
                         source_type, applied_at)
                        VALUES ($1, $2, $3, $4, $5::jsonb, $6, $7::timestamp)
                    )", character_id, effect.effect_id, *effect.duration_seconds,
                        effect.strength, effect.modifier_data, effect.source_type,
                        applied_at_str);
                } else if (effect.source_id.has_value()) {
                    txn.exec_params(R"(
                        INSERT INTO "CharacterEffects"
                        (character_id, effect_id, strength, modifier_data,
                         source_type, source_id, applied_at)
                        VALUES ($1, $2, $3, $4::jsonb, $5, $6, $7::timestamp)
                    )", character_id, effect.effect_id, effect.strength,
                        effect.modifier_data, effect.source_type,
                        *effect.source_id, applied_at_str);
                } else {
                    txn.exec_params(R"(
                        INSERT INTO "CharacterEffects"
                        (character_id, effect_id, strength, modifier_data,
                         source_type, applied_at)
                        VALUES ($1, $2, $3, $4::jsonb, $5, $6::timestamp)
                    )", character_id, effect.effect_id, effect.strength,
                        effect.modifier_data, effect.source_type, applied_at_str);
                }
            }
        }

        logger->info("Saved {} effects to database for character {}",
                    effects.size(), character_id);
        return Success();

    } catch (const pqxx::sql_error& e) {
        logger->error("SQL error saving character effects: {}", e.what());
        return std::unexpected(Error{ErrorCode::InternalError,
            fmt::format("Failed to save character effects: {}", e.what())});
    }
}

Result<void> delete_character_effects(pqxx::work& txn, const std::string& character_id) {
    auto logger = Log::database();
    logger->debug("Deleting all effects for character {}", character_id);

    try {
        auto result = txn.exec_params(R"(
            DELETE FROM "CharacterEffects"
            WHERE character_id = $1
        )", character_id);

        logger->debug("Deleted effects for character {}", character_id);
        return Success();

    } catch (const pqxx::sql_error& e) {
        logger->error("SQL error deleting character effects: {}", e.what());
        return std::unexpected(Error{ErrorCode::InternalError,
            fmt::format("Failed to delete character effects: {}", e.what())});
    }
}

Result<std::vector<AbilityEffectData>> load_ability_effects(pqxx::work& txn, int ability_id) {
    auto logger = Log::database();
    logger->debug("Loading effects for ability {}", ability_id);

    try {
        auto result = txn.exec_params(R"(
            SELECT
                ability_id, effect_id,
                override_params::text AS override_params,
                "order", trigger, chance_pct, condition
            FROM "AbilityEffect"
            WHERE ability_id = $1
            ORDER BY "order"
        )", ability_id);

        std::vector<AbilityEffectData> effects;
        effects.reserve(result.size());

        for (const auto& row : result) {
            AbilityEffectData effect;
            effect.ability_id = row["ability_id"].as<int>();
            effect.effect_id = row["effect_id"].as<int>();
            effect.override_params = row["override_params"].is_null() ? "{}" : row["override_params"].as<std::string>();
            effect.order = row["order"].as<int>(0);
            effect.trigger = row["trigger"].is_null() ? "on_hit" : row["trigger"].as<std::string>();
            effect.chance_percent = row["chance_pct"].as<int>(100);
            effect.condition = row["condition"].is_null() ? "" : row["condition"].as<std::string>();
            effects.push_back(effect);
        }

        logger->debug("Loaded {} effects for ability {}", effects.size(), ability_id);
        return effects;

    } catch (const pqxx::sql_error& e) {
        logger->error("SQL error loading ability effects for {}: {}", ability_id, e.what());
        return std::unexpected(Error{ErrorCode::InternalError,
            fmt::format("Failed to load ability effects: {}", e.what())});
    }
}

Result<std::vector<AbilityEffectData>> load_abilities_effects(
    pqxx::work& txn, const std::vector<int>& ability_ids) {
    auto logger = Log::database();

    if (ability_ids.empty()) {
        return std::vector<AbilityEffectData>{};
    }

    logger->debug("Loading effects for {} abilities", ability_ids.size());

    try {
        // Build PostgreSQL array literal for parameterized query
        // Using = ANY($1::int[]) is safer than dynamic IN clause
        std::string id_array = "{";
        for (size_t i = 0; i < ability_ids.size(); ++i) {
            if (i > 0) id_array += ",";
            id_array += std::to_string(ability_ids[i]);
        }
        id_array += "}";

        auto result = txn.exec_params(R"(
            SELECT
                ability_id, effect_id,
                override_params::text AS override_params,
                "order", trigger, chance_pct, condition
            FROM "AbilityEffect"
            WHERE ability_id = ANY($1::int[])
            ORDER BY ability_id, "order"
        )", id_array);

        std::vector<AbilityEffectData> effects;
        effects.reserve(result.size());

        for (const auto& row : result) {
            AbilityEffectData effect;
            effect.ability_id = row["ability_id"].as<int>();
            effect.effect_id = row["effect_id"].as<int>();
            effect.override_params = row["override_params"].is_null() ? "{}" : row["override_params"].as<std::string>();
            effect.order = row["order"].as<int>(0);
            effect.trigger = row["trigger"].is_null() ? "on_hit" : row["trigger"].as<std::string>();
            effect.chance_percent = row["chance_pct"].as<int>(100);
            effect.condition = row["condition"].is_null() ? "" : row["condition"].as<std::string>();
            effects.push_back(effect);
        }

        logger->debug("Loaded {} effects for {} abilities", effects.size(), ability_ids.size());
        return effects;

    } catch (const pqxx::sql_error& e) {
        logger->error("SQL error loading abilities effects: {}", e.what());
        return std::unexpected(Error{ErrorCode::InternalError,
            fmt::format("Failed to load abilities effects: {}", e.what())});
    }
}

// =============================================================================
// Ability Messages, Restrictions, and Damage Components
// =============================================================================

Result<std::vector<AbilityMessagesData>> load_all_ability_messages(pqxx::work& txn) {
    auto logger = Log::database();
    logger->debug("Loading all ability messages from database");

    try {
        auto result = txn.exec(R"(
            SELECT
                ability_id,
                start_to_caster, start_to_victim, start_to_room,
                success_to_caster, success_to_victim, success_to_room,
                success_to_self, success_self_room,
                fail_to_caster, fail_to_victim, fail_to_room,
                wearoff_to_target, wearoff_to_room,
                look_message
            FROM "AbilityMessages"
        )");

        std::vector<AbilityMessagesData> messages;
        messages.reserve(result.size());

        for (const auto& row : result) {
            AbilityMessagesData msg;
            msg.ability_id = row["ability_id"].as<int>();
            msg.start_to_caster = row["start_to_caster"].is_null() ? "" : row["start_to_caster"].as<std::string>();
            msg.start_to_victim = row["start_to_victim"].is_null() ? "" : row["start_to_victim"].as<std::string>();
            msg.start_to_room = row["start_to_room"].is_null() ? "" : row["start_to_room"].as<std::string>();
            msg.success_to_caster = row["success_to_caster"].is_null() ? "" : row["success_to_caster"].as<std::string>();
            msg.success_to_victim = row["success_to_victim"].is_null() ? "" : row["success_to_victim"].as<std::string>();
            msg.success_to_room = row["success_to_room"].is_null() ? "" : row["success_to_room"].as<std::string>();
            msg.success_to_self = row["success_to_self"].is_null() ? "" : row["success_to_self"].as<std::string>();
            msg.success_self_room = row["success_self_room"].is_null() ? "" : row["success_self_room"].as<std::string>();
            msg.fail_to_caster = row["fail_to_caster"].is_null() ? "" : row["fail_to_caster"].as<std::string>();
            msg.fail_to_victim = row["fail_to_victim"].is_null() ? "" : row["fail_to_victim"].as<std::string>();
            msg.fail_to_room = row["fail_to_room"].is_null() ? "" : row["fail_to_room"].as<std::string>();
            msg.wearoff_to_target = row["wearoff_to_target"].is_null() ? "" : row["wearoff_to_target"].as<std::string>();
            msg.wearoff_to_room = row["wearoff_to_room"].is_null() ? "" : row["wearoff_to_room"].as<std::string>();
            msg.look_message = row["look_message"].is_null() ? "" : row["look_message"].as<std::string>();
            messages.push_back(std::move(msg));
        }

        logger->debug("Loaded {} ability messages from database", messages.size());
        return messages;

    } catch (const pqxx::sql_error& e) {
        logger->error("SQL error loading ability messages: {}", e.what());
        return std::unexpected(Error{ErrorCode::InternalError,
            fmt::format("Failed to load ability messages: {}", e.what())});
    }
}

Result<std::optional<AbilityMessagesData>> load_ability_messages(
    pqxx::work& txn, int ability_id) {
    auto logger = Log::database();

    try {
        auto result = txn.exec_params(R"(
            SELECT
                ability_id,
                start_to_caster, start_to_victim, start_to_room,
                success_to_caster, success_to_victim, success_to_room,
                success_to_self, success_self_room,
                fail_to_caster, fail_to_victim, fail_to_room,
                wearoff_to_target, wearoff_to_room,
                look_message
            FROM "AbilityMessages"
            WHERE ability_id = $1
        )", ability_id);

        if (result.empty()) {
            return std::nullopt;
        }

        const auto& row = result[0];
        AbilityMessagesData msg;
        msg.ability_id = row["ability_id"].as<int>();
        msg.start_to_caster = row["start_to_caster"].is_null() ? "" : row["start_to_caster"].as<std::string>();
        msg.start_to_victim = row["start_to_victim"].is_null() ? "" : row["start_to_victim"].as<std::string>();
        msg.start_to_room = row["start_to_room"].is_null() ? "" : row["start_to_room"].as<std::string>();
        msg.success_to_caster = row["success_to_caster"].is_null() ? "" : row["success_to_caster"].as<std::string>();
        msg.success_to_victim = row["success_to_victim"].is_null() ? "" : row["success_to_victim"].as<std::string>();
        msg.success_to_room = row["success_to_room"].is_null() ? "" : row["success_to_room"].as<std::string>();
        msg.success_to_self = row["success_to_self"].is_null() ? "" : row["success_to_self"].as<std::string>();
        msg.success_self_room = row["success_self_room"].is_null() ? "" : row["success_self_room"].as<std::string>();
        msg.fail_to_caster = row["fail_to_caster"].is_null() ? "" : row["fail_to_caster"].as<std::string>();
        msg.fail_to_victim = row["fail_to_victim"].is_null() ? "" : row["fail_to_victim"].as<std::string>();
        msg.fail_to_room = row["fail_to_room"].is_null() ? "" : row["fail_to_room"].as<std::string>();
        msg.wearoff_to_target = row["wearoff_to_target"].is_null() ? "" : row["wearoff_to_target"].as<std::string>();
        msg.wearoff_to_room = row["wearoff_to_room"].is_null() ? "" : row["wearoff_to_room"].as<std::string>();
        msg.look_message = row["look_message"].is_null() ? "" : row["look_message"].as<std::string>();

        return msg;

    } catch (const pqxx::sql_error& e) {
        logger->error("SQL error loading ability messages for {}: {}", ability_id, e.what());
        return std::unexpected(Error{ErrorCode::InternalError,
            fmt::format("Failed to load ability messages: {}", e.what())});
    }
}

Result<std::vector<AbilityRestrictionsData>> load_all_ability_restrictions(pqxx::work& txn) {
    auto logger = Log::database();
    logger->debug("Loading all ability restrictions from database");

    try {
        auto result = txn.exec(R"(
            SELECT
                ability_id,
                COALESCE(array_to_json(requirements)::text, '[]') AS requirements,
                custom_requirement_lua
            FROM "AbilityRestrictions"
        )");

        std::vector<AbilityRestrictionsData> restrictions;
        restrictions.reserve(result.size());

        for (const auto& row : result) {
            AbilityRestrictionsData restr;
            restr.ability_id = row["ability_id"].as<int>();
            restr.requirements_json = row["requirements"].is_null() ? "[]" : row["requirements"].as<std::string>();
            restr.custom_requirement_lua = row["custom_requirement_lua"].is_null() ? "" : row["custom_requirement_lua"].as<std::string>();
            restrictions.push_back(std::move(restr));
        }

        logger->debug("Loaded {} ability restrictions from database", restrictions.size());
        return restrictions;

    } catch (const pqxx::sql_error& e) {
        logger->error("SQL error loading ability restrictions: {}", e.what());
        return std::unexpected(Error{ErrorCode::InternalError,
            fmt::format("Failed to load ability restrictions: {}", e.what())});
    }
}

Result<std::optional<AbilityRestrictionsData>> load_ability_restrictions(
    pqxx::work& txn, int ability_id) {
    auto logger = Log::database();

    try {
        auto result = txn.exec_params(R"(
            SELECT
                ability_id,
                COALESCE(array_to_json(requirements)::text, '[]') AS requirements,
                custom_requirement_lua
            FROM "AbilityRestrictions"
            WHERE ability_id = $1
        )", ability_id);

        if (result.empty()) {
            return std::nullopt;
        }

        const auto& row = result[0];
        AbilityRestrictionsData restr;
        restr.ability_id = row["ability_id"].as<int>();
        restr.requirements_json = row["requirements"].is_null() ? "[]" : row["requirements"].as<std::string>();
        restr.custom_requirement_lua = row["custom_requirement_lua"].is_null() ? "" : row["custom_requirement_lua"].as<std::string>();

        return restr;

    } catch (const pqxx::sql_error& e) {
        logger->error("SQL error loading ability restrictions for {}: {}", ability_id, e.what());
        return std::unexpected(Error{ErrorCode::InternalError,
            fmt::format("Failed to load ability restrictions: {}", e.what())});
    }
}

Result<std::vector<AbilityDamageComponentData>> load_all_ability_damage_components(pqxx::work& txn) {
    auto logger = Log::database();
    logger->debug("Loading all ability damage components from database");

    try {
        auto result = txn.exec(R"(
            SELECT
                ability_id, element, damage_formula, percentage, sequence
            FROM "AbilityDamageComponent"
            ORDER BY ability_id, sequence
        )");

        std::vector<AbilityDamageComponentData> components;
        components.reserve(result.size());

        for (const auto& row : result) {
            AbilityDamageComponentData comp;
            comp.ability_id = row["ability_id"].as<int>();
            comp.element = row["element"].is_null() ? "PHYSICAL" : row["element"].as<std::string>();
            comp.damage_formula = row["damage_formula"].is_null() ? "1d6" : row["damage_formula"].as<std::string>();
            comp.percentage = row["percentage"].as<int>(100);
            comp.sequence = row["sequence"].as<int>(0);
            components.push_back(std::move(comp));
        }

        logger->debug("Loaded {} ability damage components from database", components.size());
        return components;

    } catch (const pqxx::sql_error& e) {
        logger->error("SQL error loading ability damage components: {}", e.what());
        return std::unexpected(Error{ErrorCode::InternalError,
            fmt::format("Failed to load ability damage components: {}", e.what())});
    }
}

Result<std::vector<AbilityDamageComponentData>> load_ability_damage_components(
    pqxx::work& txn, int ability_id) {
    auto logger = Log::database();

    try {
        auto result = txn.exec_params(R"(
            SELECT
                ability_id, element, damage_formula, percentage, sequence
            FROM "AbilityDamageComponent"
            WHERE ability_id = $1
            ORDER BY sequence
        )", ability_id);

        std::vector<AbilityDamageComponentData> components;
        components.reserve(result.size());

        for (const auto& row : result) {
            AbilityDamageComponentData comp;
            comp.ability_id = row["ability_id"].as<int>();
            comp.element = row["element"].is_null() ? "PHYSICAL" : row["element"].as<std::string>();
            comp.damage_formula = row["damage_formula"].is_null() ? "1d6" : row["damage_formula"].as<std::string>();
            comp.percentage = row["percentage"].as<int>(100);
            comp.sequence = row["sequence"].as<int>(0);
            components.push_back(std::move(comp));
        }

        return components;

    } catch (const pqxx::sql_error& e) {
        logger->error("SQL error loading ability damage components for {}: {}", ability_id, e.what());
        return std::unexpected(Error{ErrorCode::InternalError,
            fmt::format("Failed to load ability damage components: {}", e.what())});
    }
}

Result<std::vector<AbilityClassData>> load_all_ability_classes(pqxx::work& txn) {
    auto logger = Log::database();

    try {
        // Join ClassAbilities with Class to get class names
        // Strip color codes from class names for clean display
        auto result = txn.exec(R"(
            SELECT
                ca.ability_id,
                ca.class_id,
                regexp_replace(c.name, '<[^>]+>', '', 'g') as class_name,
                ca.circle
            FROM "ClassAbilities" ca
            JOIN "Class" c ON c.id = ca.class_id
            ORDER BY ca.ability_id, c.name
        )");

        std::vector<AbilityClassData> classes;
        classes.reserve(result.size());

        for (const auto& row : result) {
            AbilityClassData cls;
            cls.ability_id = row["ability_id"].as<int>();
            cls.class_id = row["class_id"].as<int>();
            cls.class_name = row["class_name"].as<std::string>();
            cls.circle = row["circle"].as<int>();
            classes.push_back(std::move(cls));
        }

        logger->debug("Loaded {} ability-class mappings", classes.size());
        return classes;

    } catch (const pqxx::sql_error& e) {
        logger->error("SQL error loading ability classes: {}", e.what());
        return std::unexpected(Error{ErrorCode::InternalError,
            fmt::format("Failed to load ability classes: {}", e.what())});
    }
}

std::vector<AbilityRequirement> parse_ability_requirements(
    std::string_view json_str, int ability_id) {
    std::vector<AbilityRequirement> requirements;
    if (json_str.empty() || json_str == "[]") {
        return requirements;
    }

    try {
        auto j = nlohmann::json::parse(json_str);
        if (!j.is_array()) {
            return requirements;
        }

        for (const auto& item : j) {
            if (!item.is_object()) continue;

            AbilityRequirement req;
            if (item.contains("type")) {
                req.type = item["type"].get<std::string>();
            }
            if (item.contains("value")) {
                if (item["value"].is_string()) {
                    req.value = item["value"].get<std::string>();
                } else if (item["value"].is_boolean()) {
                    req.value = item["value"].get<bool>() ? "true" : "false";
                } else if (item["value"].is_number()) {
                    req.value = std::to_string(item["value"].get<int>());
                }
            }
            if (item.contains("negated")) {
                req.negated = item["negated"].get<bool>();
            }
            requirements.push_back(std::move(req));
        }
    } catch (const nlohmann::json::exception& e) {
        // Include ability ID and truncated JSON in error for easier debugging
        std::string json_preview(json_str.substr(0, std::min(json_str.size(), size_t(200))));
        if (json_str.size() > 200) json_preview += "...";
        Log::warn("Failed to parse ability requirements JSON for ability_id={}: {} | JSON: {}",
                  ability_id, e.what(), json_preview);
    }

    return requirements;
}

// =============================================================================
// Character/Player System Queries
// =============================================================================

namespace {
    // Helper to parse PostgreSQL array to vector<string>
    std::vector<std::string> parse_pg_string_array(const std::string& pg_array) {
        std::vector<std::string> result;
        if (pg_array.empty() || pg_array == "{}") {
            return result;
        }

        // Remove outer braces
        std::string inner = pg_array;
        if (inner.front() == '{') inner = inner.substr(1);
        if (inner.back() == '}') inner = inner.substr(0, inner.size() - 1);

        // Split by comma (simple parser, doesn't handle quotes perfectly)
        size_t start = 0;
        bool in_quotes = false;
        for (size_t i = 0; i < inner.size(); ++i) {
            if (inner[i] == '"') {
                in_quotes = !in_quotes;
            } else if (inner[i] == ',' && !in_quotes) {
                std::string item = inner.substr(start, i - start);
                // Remove quotes if present
                if (item.front() == '"') item = item.substr(1);
                if (item.back() == '"') item = item.substr(0, item.size() - 1);
                result.push_back(item);
                start = i + 1;
            }
        }
        // Last item
        if (start < inner.size()) {
            std::string item = inner.substr(start);
            if (!item.empty()) {
                if (item.front() == '"') item = item.substr(1);
                if (!item.empty() && item.back() == '"') item = item.substr(0, item.size() - 1);
                result.push_back(item);
            }
        }
        return result;
    }

    // Helper to escape a string for PostgreSQL array literal
    // Escapes backslashes and double quotes per PostgreSQL array syntax
    std::string escape_pg_array_element(const std::string& str) {
        std::string escaped;
        escaped.reserve(str.size() + 8);  // Preallocate with some extra space
        for (char c : str) {
            if (c == '\\' || c == '"') {
                escaped += '\\';
            }
            escaped += c;
        }
        return escaped;
    }

    // Helper to convert vector<string> to PostgreSQL array literal
    // Properly escapes special characters to prevent SQL injection
    std::string to_pg_array(const std::vector<std::string>& vec) {
        if (vec.empty()) {
            return "{}";
        }
        std::string result = "{";
        for (size_t i = 0; i < vec.size(); ++i) {
            if (i > 0) result += ",";
            result += "\"" + escape_pg_array_element(vec[i]) + "\"";
        }
        result += "}";
        return result;
    }

    // Generate a UUID for new characters
    std::string generate_uuid() {
        // Simple UUID generation using random
        static std::random_device rd;
        static std::mt19937_64 gen(rd());
        static std::uniform_int_distribution<uint64_t> dist;

        uint64_t a = dist(gen);
        uint64_t b = dist(gen);

        // Format as UUID string using fmt::format
        return fmt::format("{:08x}-{:04x}-{:04x}-{:04x}-{:012x}",
            static_cast<unsigned int>(a >> 32),
            static_cast<unsigned int>((a >> 16) & 0xFFFF),
            static_cast<unsigned int>(a & 0xFFFF),
            static_cast<unsigned int>(b >> 48),
            static_cast<unsigned long long>(b & 0xFFFFFFFFFFFFULL));
    }

    // Bcrypt-style password hashing using crypt() with SHA-512
    // Format: $6$<salt>$<hash> (SHA-512 crypt)
    std::string hash_password(const std::string& password) {
        // Generate random salt for SHA-512 crypt
        std::array<unsigned char, 16> salt_bytes{};
        RAND_bytes(salt_bytes.data(), salt_bytes.size());

        // Encode salt as base64-like string
        constexpr std::string_view b64 = "./0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
        std::string salt = "$6$";  // SHA-512 prefix
        for (const auto& byte : salt_bytes) {
            salt += b64[byte % 64];
        }
        salt += "$";

        // Use crypt_r for thread safety
        struct crypt_data data{};  // Zero-initialize with {}
        char* result = crypt_r(password.c_str(), salt.c_str(), &data);

        if (result == nullptr) {
            // Fallback to SHA-256 if crypt_r fails (should not happen on modern Linux)
            auto logger = Log::database();
            logger->error("crypt_r failed, using SHA-256 fallback hash");

            // Generate 16-byte salt
            std::array<unsigned char, 16> fallback_salt{};
            RAND_bytes(fallback_salt.data(), fallback_salt.size());

            // Build salted password: salt + password
            std::string salted = std::string(fallback_salt.begin(), fallback_salt.end()) + password;

            // Compute SHA-256 hash using OpenSSL EVP
            std::array<unsigned char, 32> hash_bytes{};
            unsigned int hash_len = 0;
            EVP_MD_CTX* ctx = EVP_MD_CTX_new();
            if (ctx) {
                EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr);
                EVP_DigestUpdate(ctx, salted.data(), salted.size());
                EVP_DigestFinal_ex(ctx, hash_bytes.data(), &hash_len);
                EVP_MD_CTX_free(ctx);
            }

            // Format as: fallback$<salt_hex>$<hash_hex>
            std::string salt_hex;
            std::string hash_hex;
            for (auto b : fallback_salt) {
                salt_hex += fmt::format("{:02x}", b);
            }
            for (unsigned int i = 0; i < hash_len; ++i) {
                hash_hex += fmt::format("{:02x}", hash_bytes[i]);
            }
            return fmt::format("fallback${}${}", salt_hex, hash_hex);
        }

        return std::string(result);
    }

    // Detect password hash type
    enum class HashType {
        LegacyCrypt,     // 10-13 character DES/Unix crypt
        SHA512Crypt,     // $6$...$... format
        BcryptHash,      // $2a$, $2b$, $2y$ format (bcrypt)
        FallbackHash,    // fallback$... format
        Unknown
    };

    HashType detect_hash_type(const std::string& hash) {
        if (hash.starts_with("$6$")) {
            return HashType::SHA512Crypt;
        }
        // Bcrypt hashes: $2a$, $2b$, or $2y$ prefix (60 characters total)
        if ((hash.starts_with("$2a$") || hash.starts_with("$2b$") || hash.starts_with("$2y$"))
            && hash.length() == 60) {
            return HashType::BcryptHash;
        }
        if (hash.starts_with("fallback$")) {
            return HashType::FallbackHash;
        }
        // Legacy Unix crypt hashes are 10-13 characters
        if (hash.length() >= 10 && hash.length() <= 13) {
            return HashType::LegacyCrypt;
        }
        return HashType::Unknown;
    }

    // Verify password against stored hash (supports multiple formats)
    bool verify_password_hash(const std::string& password, const std::string& stored_hash) {
        HashType type = detect_hash_type(stored_hash);

        switch (type) {
            case HashType::SHA512Crypt: {
                // Modern SHA-512 crypt - use crypt_r with stored hash as salt
                struct crypt_data data{};  // Zero-initialize with {}
                char* result = crypt_r(password.c_str(), stored_hash.c_str(), &data);
                return result && stored_hash == result;
            }

            case HashType::BcryptHash: {
                // Bcrypt hash ($2a$, $2b$, $2y$ format) - use crypt_r if supported
                // libxcrypt on modern Linux supports bcrypt via crypt_r
                struct crypt_data data{};  // Zero-initialize with {}
                char* result = crypt_r(password.c_str(), stored_hash.c_str(), &data);
                if (!result) {
                    // crypt_r doesn't support bcrypt on this system
                    auto logger = Log::database();
                    logger->warn("Bcrypt hash verification failed - crypt_r may not support bcrypt on this system");
                    return false;
                }
                return stored_hash == result;
            }

            case HashType::LegacyCrypt: {
                // Legacy Unix crypt (DES) - 10-13 character hash
                // Salt is first 2 characters of the hash
                struct crypt_data data{};  // Zero-initialize with {}
                char* result = crypt_r(password.c_str(), stored_hash.c_str(), &data);
                if (!result) return false;
                // Legacy hashes compare first 10 characters
                return std::strncmp(result, stored_hash.c_str(), 10) == 0;
            }

            case HashType::FallbackHash: {
                // SHA-256 fallback hash: fallback$<salt_hex>$<hash_hex>
                // Parse the stored hash to extract salt
                auto first_dollar = stored_hash.find('$');
                if (first_dollar == std::string::npos) return false;
                auto second_dollar = stored_hash.find('$', first_dollar + 1);
                if (second_dollar == std::string::npos) {
                    // Old format: fallback$<hash_hex> (legacy std::hash - deprecated)
                    // For backwards compatibility only - will be upgraded on next login
                    std::size_t legacy_hash = std::hash<std::string>{}(password);
                    return stored_hash == fmt::format("fallback${:016x}", legacy_hash);
                }

                // New format: fallback$<salt_hex>$<hash_hex>
                std::string salt_hex = stored_hash.substr(first_dollar + 1, second_dollar - first_dollar - 1);
                std::string stored_hash_hex = stored_hash.substr(second_dollar + 1);

                // Convert salt from hex to bytes
                std::string salt_bytes;
                for (size_t i = 0; i + 1 < salt_hex.size(); i += 2) {
                    int byte = std::stoi(salt_hex.substr(i, 2), nullptr, 16);
                    salt_bytes += static_cast<char>(byte);
                }

                // Compute hash of salt + password
                std::string salted = salt_bytes + password;
                std::array<unsigned char, 32> computed_hash{};
                unsigned int hash_len = 0;
                EVP_MD_CTX* ctx = EVP_MD_CTX_new();
                if (ctx) {
                    EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr);
                    EVP_DigestUpdate(ctx, salted.data(), salted.size());
                    EVP_DigestFinal_ex(ctx, computed_hash.data(), &hash_len);
                    EVP_MD_CTX_free(ctx);
                }

                // Convert computed hash to hex and compare
                std::string computed_hex;
                for (unsigned int i = 0; i < hash_len; ++i) {
                    computed_hex += fmt::format("{:02x}", computed_hash[i]);
                }
                return stored_hash_hex == computed_hex;
            }

            case HashType::Unknown:
            default: {
                // SECURITY: Reject unknown hash formats instead of direct comparison
                // Direct comparison would allow plaintext passwords to work, which is dangerous
                auto logger = Log::database();
                logger->error("Password verification failed: unknown hash format (length {})",
                             stored_hash.size());
                return false;
            }
        }
    }

    // Check if a password hash should be upgraded to modern format
    bool should_upgrade_password(const std::string& stored_hash) {
        HashType type = detect_hash_type(stored_hash);
        // Upgrade legacy crypt and fallback hashes to SHA-512
        return type == HashType::LegacyCrypt || type == HashType::FallbackHash;
    }
}

Result<bool> character_exists(pqxx::work& txn, const std::string& name) {
    auto logger = Log::database();
    logger->debug("Checking if character '{}' exists", name);

    try {
        auto result = txn.exec_params(R"(
            SELECT 1 FROM "Characters" WHERE UPPER(name) = UPPER($1) LIMIT 1
        )", name);

        return !result.empty();

    } catch (const pqxx::sql_error& e) {
        logger->error("SQL error checking character existence: {}", e.what());
        return std::unexpected(Error{ErrorCode::InternalError,
            fmt::format("Failed to check character existence: {}", e.what())});
    }
}

Result<CharacterData> load_character_by_name(pqxx::work& txn, const std::string& name) {
    auto logger = Log::database();
    logger->debug("Loading character by name: {}", name);

    try {
        auto result = txn.exec_params(R"(
            SELECT
                c.id, c.name, c.level, c.alignment,
                c.strength, c.intelligence, c.wisdom, c.dexterity, c.constitution, c.charisma, c.luck,
                c.hit_points, c.hit_points_max, c.stamina, c.stamina_max,
                c.wealth, c.bank_wealth,
                c.password_hash, c.race_type, c.gender, c.player_class, c.class_id,
                c.current_room_zone_id, c.current_room_id,
                c.recall_room_zone_id, c.recall_room_id,
                c.hit_roll, c.damage_roll, c.armor_class,
                c.last_login, c.time_played, c.is_online,
                c.hunger, c.thirst,
                c.description, c.title,
                c.prompt, c.page_length, c.wimpy_threshold,
                c.player_flags,
                c.experience, c.skill_points, c.position,
                c.created_at, c.updated_at,
                c.user_id,
                COALESCE(u.account_wealth, 0) as account_wealth
            FROM "Characters" c
            LEFT JOIN "Users" u ON c.user_id = u.id
            WHERE UPPER(c.name) = UPPER($1)
        )", name);

        if (result.empty()) {
            return std::unexpected(Error{ErrorCode::NotFound,
                fmt::format("Character '{}' not found", name)});
        }

        const auto& row = result[0];
        CharacterData character;

        character.id = row["id"].as<std::string>();
        character.name = row["name"].as<std::string>();
        character.level = row["level"].as<int>(1);
        character.alignment = row["alignment"].as<int>(0);

        // Stats
        character.strength = row["strength"].as<int>(13);
        character.intelligence = row["intelligence"].as<int>(13);
        character.wisdom = row["wisdom"].as<int>(13);
        character.dexterity = row["dexterity"].as<int>(13);
        character.constitution = row["constitution"].as<int>(13);
        character.charisma = row["charisma"].as<int>(13);
        character.luck = row["luck"].as<int>(13);

        // Vitals
        character.hit_points = row["hit_points"].as<int>(100);
        character.hit_points_max = row["hit_points_max"].as<int>(100);
        character.stamina = row["stamina"].as<int>(100);
        character.stamina_max = row["stamina_max"].as<int>(100);

        // Currency (stored as copper)
        character.wealth = row["wealth"].as<long>(0);
        character.bank_wealth = row["bank_wealth"].as<long>(0);
        character.account_wealth = row["account_wealth"].as<long>(0);

        // User link
        if (!row["user_id"].is_null()) {
            character.user_id = row["user_id"].as<std::string>();
        }

        // Character info
        character.password_hash = row["password_hash"].as<std::string>("");
        character.race_type = row["race_type"].as<std::string>("human");
        character.gender = row["gender"].as<std::string>("neutral");
        if (!row["player_class"].is_null()) {
            character.player_class = row["player_class"].as<std::string>();
        }
        if (!row["class_id"].is_null()) {
            character.class_id = row["class_id"].as<int>();
        }

        // Location (composite keys)
        if (!row["current_room_zone_id"].is_null()) {
            character.current_room_zone_id = row["current_room_zone_id"].as<int>();
        }
        if (!row["current_room_id"].is_null()) {
            character.current_room_id = row["current_room_id"].as<int>();
        }
        if (!row["recall_room_zone_id"].is_null()) {
            character.recall_room_zone_id = row["recall_room_zone_id"].as<int>();
        }
        if (!row["recall_room_id"].is_null()) {
            character.recall_room_id = row["recall_room_id"].as<int>();
        }

        // Combat stats
        character.hit_roll = row["hit_roll"].as<int>(0);
        character.damage_roll = row["damage_roll"].as<int>(0);
        character.armor_class = row["armor_class"].as<int>(10);

        // Session
        character.time_played = row["time_played"].as<int>(0);
        character.is_online = row["is_online"].as<bool>(false);

        // Needs
        character.hunger = row["hunger"].as<int>(0);
        character.thirst = row["thirst"].as<int>(0);

        // Description/Title
        if (!row["description"].is_null()) {
            character.description = row["description"].as<std::string>();
        }
        if (!row["title"].is_null()) {
            character.title = row["title"].as<std::string>();
        }

        // Preferences
        character.prompt = row["prompt"].as<std::string>("<%%h/%%Hhp %%v/%%Vmv>");
        character.page_length = row["page_length"].as<int>(25);
        character.wimpy_threshold = row["wimpy_threshold"].as<int>(0);

        // Flags (PostgreSQL arrays)
        if (!row["player_flags"].is_null()) {
            character.player_flags = parse_pg_string_array(row["player_flags"].as<std::string>());
        }

        // Experience
        character.experience = row["experience"].as<int>(0);
        character.skill_points = row["skill_points"].as<int>(0);

        // Position (for ghost state persistence)
        character.position = row["position"].as<std::string>("STANDING");

        logger->debug("Loaded character '{}' (id: {})", character.name, character.id);
        return character;

    } catch (const pqxx::sql_error& e) {
        logger->error("SQL error loading character '{}': {}", name, e.what());
        return std::unexpected(Error{ErrorCode::InternalError,
            fmt::format("Failed to load character: {}", e.what())});
    }
}

Result<CharacterData> load_character_by_id(pqxx::work& txn, const std::string& id) {
    auto logger = Log::database();
    logger->debug("Loading character by ID: {}", id);

    try {
        auto result = txn.exec_params(R"(
            SELECT
                c.id, c.name, c.level, c.alignment,
                c.strength, c.intelligence, c.wisdom, c.dexterity, c.constitution, c.charisma, c.luck,
                c.hit_points, c.hit_points_max, c.stamina, c.stamina_max,
                c.wealth, c.bank_wealth,
                c.password_hash, c.race_type, c.gender, c.player_class, c.class_id,
                c.current_room_zone_id, c.current_room_id,
                c.recall_room_zone_id, c.recall_room_id,
                c.hit_roll, c.damage_roll, c.armor_class,
                c.last_login, c.time_played, c.is_online,
                c.hunger, c.thirst,
                c.description, c.title,
                c.prompt, c.page_length, c.wimpy_threshold,
                c.player_flags,
                c.experience, c.skill_points, c.position,
                c.created_at, c.updated_at,
                c.user_id,
                COALESCE(u.account_wealth, 0) as account_wealth
            FROM "Characters" c
            LEFT JOIN "Users" u ON c.user_id = u.id
            WHERE c.id = $1
        )", id);

        if (result.empty()) {
            return std::unexpected(Error{ErrorCode::NotFound,
                fmt::format("Character with ID '{}' not found", id)});
        }

        // Same field parsing as load_character_by_name
        const auto& row = result[0];
        CharacterData character;

        character.id = row["id"].as<std::string>();
        character.name = row["name"].as<std::string>();
        character.level = row["level"].as<int>(1);
        character.alignment = row["alignment"].as<int>(0);
        character.strength = row["strength"].as<int>(13);
        character.intelligence = row["intelligence"].as<int>(13);
        character.wisdom = row["wisdom"].as<int>(13);
        character.dexterity = row["dexterity"].as<int>(13);
        character.constitution = row["constitution"].as<int>(13);
        character.charisma = row["charisma"].as<int>(13);
        character.luck = row["luck"].as<int>(13);
        character.hit_points = row["hit_points"].as<int>(100);
        character.hit_points_max = row["hit_points_max"].as<int>(100);
        character.stamina = row["stamina"].as<int>(100);
        character.stamina_max = row["stamina_max"].as<int>(100);
        character.wealth = row["wealth"].as<long>(0);
        character.bank_wealth = row["bank_wealth"].as<long>(0);
        character.account_wealth = row["account_wealth"].as<long>(0);
        if (!row["user_id"].is_null()) {
            character.user_id = row["user_id"].as<std::string>();
        }
        character.password_hash = row["password_hash"].as<std::string>("");
        character.race_type = row["race_type"].as<std::string>("human");
        character.gender = row["gender"].as<std::string>("neutral");
        if (!row["player_class"].is_null()) {
            character.player_class = row["player_class"].as<std::string>();
        }
        if (!row["class_id"].is_null()) {
            character.class_id = row["class_id"].as<int>();
        }
        if (!row["current_room_zone_id"].is_null()) {
            character.current_room_zone_id = row["current_room_zone_id"].as<int>();
        }
        if (!row["current_room_id"].is_null()) {
            character.current_room_id = row["current_room_id"].as<int>();
        }
        if (!row["recall_room_zone_id"].is_null()) {
            character.recall_room_zone_id = row["recall_room_zone_id"].as<int>();
        }
        if (!row["recall_room_id"].is_null()) {
            character.recall_room_id = row["recall_room_id"].as<int>();
        }
        character.hit_roll = row["hit_roll"].as<int>(0);
        character.damage_roll = row["damage_roll"].as<int>(0);
        character.armor_class = row["armor_class"].as<int>(10);
        character.time_played = row["time_played"].as<int>(0);
        character.is_online = row["is_online"].as<bool>(false);
        character.hunger = row["hunger"].as<int>(0);
        character.thirst = row["thirst"].as<int>(0);
        if (!row["description"].is_null()) {
            character.description = row["description"].as<std::string>();
        }
        if (!row["title"].is_null()) {
            character.title = row["title"].as<std::string>();
        }
        character.prompt = row["prompt"].as<std::string>("<%%h/%%Hhp %%v/%%Vmv>");
        character.page_length = row["page_length"].as<int>(25);
        character.wimpy_threshold = row["wimpy_threshold"].as<int>(0);
        if (!row["player_flags"].is_null()) {
            character.player_flags = parse_pg_string_array(row["player_flags"].as<std::string>());
        }
        character.experience = row["experience"].as<int>(0);
        character.skill_points = row["skill_points"].as<int>(0);
        character.position = row["position"].as<std::string>("STANDING");

        logger->debug("Loaded character '{}' by ID", character.name);
        return character;

    } catch (const pqxx::sql_error& e) {
        logger->error("SQL error loading character by ID '{}': {}", id, e.what());
        return std::unexpected(Error{ErrorCode::InternalError,
            fmt::format("Failed to load character: {}", e.what())});
    }
}

Result<bool> verify_character_password(
    pqxx::work& txn,
    const std::string& name,
    const std::string& password) {
    auto logger = Log::database();
    logger->debug("Verifying password for character '{}'", name);

    try {
        auto result = txn.exec_params(R"(
            SELECT id, password_hash FROM "Characters" WHERE UPPER(name) = UPPER($1)
        )", name);

        if (result.empty()) {
            return std::unexpected(Error{ErrorCode::NotFound,
                fmt::format("Character '{}' not found", name)});
        }

        std::string character_id = result[0]["id"].as<std::string>();
        std::string stored_hash = result[0]["password_hash"].as<std::string>("");
        bool matches = verify_password_hash(password, stored_hash);

        if (matches && should_upgrade_password(stored_hash)) {
            // Upgrade legacy password hash to modern SHA-512 format
            std::string new_hash = hash_password(password);
            txn.exec_params(R"(
                UPDATE "Characters" SET password_hash = $1, updated_at = NOW()
                WHERE id = $2
            )", new_hash, character_id);
            logger->debug("Upgraded password hash for character '{}' from legacy to SHA-512", name);
        }

        logger->debug("Password verification for '{}': {}", name, matches ? "success" : "failed");
        return matches;

    } catch (const pqxx::sql_error& e) {
        logger->error("SQL error verifying password for '{}': {}", name, e.what());
        return std::unexpected(Error{ErrorCode::InternalError,
            fmt::format("Failed to verify password: {}", e.what())});
    }
}

Result<CharacterData> create_character(
    pqxx::work& txn,
    const std::string& name,
    const std::string& password,
    const std::string& player_class,
    const std::string& race) {
    auto logger = Log::database();
    logger->info("Creating new character: {} ({} {})", name, race, player_class);

    try {
        // Generate UUID
        std::string character_id = generate_uuid();
        std::string password_hash_str = hash_password(password);

        // Convert race to lowercase for database storage
        std::string race_lower = to_lowercase(race);

        txn.exec_params(R"(
            INSERT INTO "Characters" (
                id, name, password_hash, player_class, race_type, race, level,
                strength, intelligence, wisdom, dexterity, constitution, charisma, luck,
                hit_points, hit_points_max, stamina, stamina_max,
                created_at, updated_at
            ) VALUES (
                $1, $2, $3, $4, $5, $6, 1,
                13, 13, 13, 13, 13, 13, 13,
                100, 100, 100, 100,
                NOW(), NOW()
            )
        )", character_id, name, password_hash_str, player_class, race_lower,
            race == "Human" ? "HUMAN" : race == "Elf" ? "ELF" : race == "Dwarf" ? "DWARF" : "HALFLING");

        // Return the created character data
        CharacterData character;
        character.id = character_id;
        character.name = name;
        character.password_hash = password_hash_str;
        character.player_class = player_class;
        character.race_type = race_lower;
        character.level = 1;
        character.strength = 13;
        character.intelligence = 13;
        character.wisdom = 13;
        character.dexterity = 13;
        character.constitution = 13;
        character.charisma = 13;
        character.luck = 13;
        character.hit_points = 100;
        character.hit_points_max = 100;
        character.stamina = 100;
        character.stamina_max = 100;

        logger->info("Created character '{}' with ID {}", name, character_id);
        return character;

    } catch (const pqxx::sql_error& e) {
        logger->error("SQL error creating character '{}': {}", name, e.what());
        return std::unexpected(Error{ErrorCode::InternalError,
            fmt::format("Failed to create character: {}", e.what())});
    }
}

Result<void> save_character(pqxx::work& txn, const CharacterData& character) {
    auto logger = Log::database();
    logger->debug("Saving character '{}' (id: {})", character.name, character.id);

    try {
        // Convert player_flags to PostgreSQL array format
        std::string player_flags_array = to_pg_array(character.player_flags);

        txn.exec_params(R"(
            UPDATE "Characters" SET
                level = $2, alignment = $3,
                strength = $4, intelligence = $5, wisdom = $6, dexterity = $7,
                constitution = $8, charisma = $9, luck = $10,
                hit_points = $11, hit_points_max = $12, stamina = $13, stamina_max = $14,
                wealth = $15, bank_wealth = $16,
                current_room_zone_id = $17, current_room_id = $18,
                recall_room_zone_id = $19, recall_room_id = $20,
                hit_roll = $21, damage_roll = $22, armor_class = $23,
                time_played = $24, hunger = $25, thirst = $26,
                experience = $27, skill_points = $28, position = $29::"Position",
                title = $30, description = $31,
                prompt = $32, page_length = $33, wimpy_threshold = $34,
                player_flags = $35::"PlayerFlag"[],
                updated_at = NOW()
            WHERE id = $1
        )",
            character.id,
            character.level, character.alignment,
            character.strength, character.intelligence, character.wisdom, character.dexterity,
            character.constitution, character.charisma, character.luck,
            character.hit_points, character.hit_points_max, character.stamina, character.stamina_max,
            character.wealth, character.bank_wealth,
            character.current_room_zone_id.value_or(0), character.current_room_id.value_or(0),
            character.recall_room_zone_id.value_or(0), character.recall_room_id.value_or(0),
            character.hit_roll, character.damage_roll, character.armor_class,
            character.time_played, character.hunger, character.thirst,
            character.experience, character.skill_points, character.position,
            character.title, character.description,
            character.prompt, character.page_length, character.wimpy_threshold,
            player_flags_array
        );

        logger->debug("Saved character '{}' successfully", character.name);
        return {};

    } catch (const pqxx::sql_error& e) {
        logger->error("SQL error saving character '{}': {}", character.name, e.what());
        return std::unexpected(Error{ErrorCode::InternalError,
            fmt::format("Failed to save character: {}", e.what())});
    }
}

Result<void> set_character_online(
    pqxx::work& txn,
    const std::string& character_id,
    bool is_online) {
    auto logger = Log::database();
    logger->debug("Setting character {} online status to {}", character_id, is_online);

    try {
        txn.exec_params(R"(
            UPDATE "Characters" SET is_online = $2, updated_at = NOW() WHERE id = $1
        )", character_id, is_online);

        return {};

    } catch (const pqxx::sql_error& e) {
        logger->error("SQL error updating online status: {}", e.what());
        return std::unexpected(Error{ErrorCode::InternalError,
            fmt::format("Failed to update online status: {}", e.what())});
    }
}

Result<void> update_last_login(pqxx::work& txn, const std::string& character_id) {
    auto logger = Log::database();
    logger->debug("Updating last login for character {}", character_id);

    try {
        txn.exec_params(R"(
            UPDATE "Characters" SET last_login = NOW(), updated_at = NOW() WHERE id = $1
        )", character_id);

        return {};

    } catch (const pqxx::sql_error& e) {
        logger->error("SQL error updating last login: {}", e.what());
        return std::unexpected(Error{ErrorCode::InternalError,
            fmt::format("Failed to update last login: {}", e.what())});
    }
}

Result<void> save_character_location(
    pqxx::work& txn,
    const std::string& character_id,
    int zone_id,
    int room_id) {
    auto logger = Log::database();
    logger->debug("Saving location for character {}: zone {} room {}", character_id, zone_id, room_id);

    try {
        txn.exec_params(R"(
            UPDATE "Characters" SET
                current_room_zone_id = $2, current_room_id = $3,
                updated_at = NOW()
            WHERE id = $1
        )", character_id, zone_id, room_id);

        return {};

    } catch (const pqxx::sql_error& e) {
        logger->error("SQL error saving character location: {}", e.what());
        return std::unexpected(Error{ErrorCode::InternalError,
            fmt::format("Failed to save character location: {}", e.what())});
    }
}

// =============================================================================
// User/Account System Queries
// =============================================================================

namespace {
    // Parse UserRole from database string
    UserRole parse_user_role(const std::string& role_str) {
        if (role_str == "PLAYER") return UserRole::Player;
        if (role_str == "IMMORTAL") return UserRole::Immortal;
        if (role_str == "BUILDER") return UserRole::Builder;
        if (role_str == "HEAD_BUILDER") return UserRole::HeadBuilder;
        if (role_str == "CODER") return UserRole::Coder;
        if (role_str == "GOD") return UserRole::God;
        return UserRole::Player;  // Default
    }

    // Helper to populate UserData from a database row
    UserData row_to_user_data(const pqxx::row& row) {
        UserData user;
        user.id = row["id"].as<std::string>();
        user.email = row["email"].as<std::string>();
        user.username = row["username"].as<std::string>();
        user.password_hash = row["password_hash"].as<std::string>("");
        user.role = parse_user_role(row["role"].as<std::string>("PLAYER"));
        user.failed_login_attempts = row["failed_login_attempts"].as<int>(0);
        // Timestamps would need proper parsing - leaving as default for now
        return user;
    }
}

Result<bool> user_exists(pqxx::work& txn, const std::string& username_or_email) {
    auto logger = Log::database();
    logger->debug("Checking if user '{}' exists", username_or_email);

    try {
        auto result = txn.exec_params(R"(
            SELECT 1 FROM "Users"
            WHERE UPPER(username) = UPPER($1) OR UPPER(email) = UPPER($1)
            LIMIT 1
        )", username_or_email);

        return !result.empty();

    } catch (const pqxx::sql_error& e) {
        logger->error("SQL error checking user existence: {}", e.what());
        return std::unexpected(Error{ErrorCode::InternalError,
            fmt::format("Failed to check user existence: {}", e.what())});
    }
}

Result<UserData> load_user_by_username(pqxx::work& txn, const std::string& username) {
    auto logger = Log::database();
    logger->debug("Loading user by username: {}", username);

    try {
        auto result = txn.exec_params(R"(
            SELECT
                id, email, username, password_hash, role,
                failed_login_attempts, locked_until, last_login_at,
                created_at, updated_at
            FROM "Users"
            WHERE UPPER(username) = UPPER($1)
        )", username);

        if (result.empty()) {
            return std::unexpected(Error{ErrorCode::NotFound,
                fmt::format("User '{}' not found", username)});
        }

        auto user = row_to_user_data(result[0]);
        logger->debug("Loaded user '{}' (id: {})", user.username, user.id);
        return user;

    } catch (const pqxx::sql_error& e) {
        logger->error("SQL error loading user '{}': {}", username, e.what());
        return std::unexpected(Error{ErrorCode::InternalError,
            fmt::format("Failed to load user: {}", e.what())});
    }
}

Result<UserData> load_user_by_email(pqxx::work& txn, const std::string& email) {
    auto logger = Log::database();
    logger->debug("Loading user by email: {}", email);

    try {
        auto result = txn.exec_params(R"(
            SELECT
                id, email, username, password_hash, role,
                failed_login_attempts, locked_until, last_login_at,
                created_at, updated_at
            FROM "Users"
            WHERE UPPER(email) = UPPER($1)
        )", email);

        if (result.empty()) {
            return std::unexpected(Error{ErrorCode::NotFound,
                fmt::format("User with email '{}' not found", email)});
        }

        auto user = row_to_user_data(result[0]);
        logger->debug("Loaded user '{}' by email", user.username);
        return user;

    } catch (const pqxx::sql_error& e) {
        logger->error("SQL error loading user by email '{}': {}", email, e.what());
        return std::unexpected(Error{ErrorCode::InternalError,
            fmt::format("Failed to load user: {}", e.what())});
    }
}

Result<UserData> load_user_by_id(pqxx::work& txn, const std::string& id) {
    auto logger = Log::database();
    logger->debug("Loading user by ID: {}", id);

    try {
        auto result = txn.exec_params(R"(
            SELECT
                id, email, username, password_hash, role,
                failed_login_attempts, locked_until, last_login_at,
                created_at, updated_at
            FROM "Users"
            WHERE id = $1
        )", id);

        if (result.empty()) {
            return std::unexpected(Error{ErrorCode::NotFound,
                fmt::format("User with ID '{}' not found", id)});
        }

        auto user = row_to_user_data(result[0]);
        logger->debug("Loaded user '{}' by ID", user.username);
        return user;

    } catch (const pqxx::sql_error& e) {
        logger->error("SQL error loading user by ID '{}': {}", id, e.what());
        return std::unexpected(Error{ErrorCode::InternalError,
            fmt::format("Failed to load user: {}", e.what())});
    }
}

Result<bool> verify_user_password(
    pqxx::work& txn,
    const std::string& username_or_email,
    const std::string& password) {
    auto logger = Log::database();
    logger->debug("Verifying password for user '{}'", username_or_email);

    try {
        // Check if account is locked
        auto result = txn.exec_params(R"(
            SELECT id, password_hash, locked_until
            FROM "Users"
            WHERE UPPER(username) = UPPER($1) OR UPPER(email) = UPPER($1)
        )", username_or_email);

        if (result.empty()) {
            return std::unexpected(Error{ErrorCode::NotFound,
                fmt::format("User '{}' not found", username_or_email)});
        }

        std::string user_id = result[0]["id"].as<std::string>();
        std::string stored_hash = result[0]["password_hash"].as<std::string>("");

        // Check if account is locked
        if (!result[0]["locked_until"].is_null()) {
            // For simplicity, we'll just log this - proper timestamp parsing would be needed
            logger->warn("User '{}' account may be locked", username_or_email);
        }

        bool matches = verify_password_hash(password, stored_hash);

        if (matches) {
            // Upgrade legacy password hash to modern SHA-512 format if needed
            if (should_upgrade_password(stored_hash)) {
                std::string new_hash = hash_password(password);
                txn.exec_params(R"(
                    UPDATE "Users" SET password_hash = $1, updated_at = NOW()
                    WHERE id = $2
                )", new_hash, user_id);
                logger->debug("Upgraded password hash for user '{}' from legacy to SHA-512", username_or_email);
            }
        }

        logger->debug("Password verification for user '{}': {}", username_or_email, matches ? "success" : "failed");
        return matches;

    } catch (const pqxx::sql_error& e) {
        logger->error("SQL error verifying password for user '{}': {}", username_or_email, e.what());
        return std::unexpected(Error{ErrorCode::InternalError,
            fmt::format("Failed to verify password: {}", e.what())});
    }
}

Result<std::vector<CharacterSummary>> get_user_characters(
    pqxx::work& txn,
    const std::string& user_id) {
    auto logger = Log::database();
    logger->debug("Loading characters for user {}", user_id);

    try {
        auto result = txn.exec_params(R"(
            SELECT
                id, name, level, player_class, race_type, is_online, last_login
            FROM "Characters"
            WHERE user_id = $1
            ORDER BY last_login DESC NULLS LAST, name ASC
        )", user_id);

        std::vector<CharacterSummary> characters;
        characters.reserve(result.size());

        for (const auto& row : result) {
            CharacterSummary summary;
            summary.id = row["id"].as<std::string>();
            summary.name = row["name"].as<std::string>();
            summary.level = row["level"].as<int>(1);
            summary.player_class = row["player_class"].is_null() ? "Unknown" : row["player_class"].as<std::string>();
            summary.race_type = row["race_type"].is_null() ? "human" : row["race_type"].as<std::string>();
            summary.is_online = row["is_online"].as<bool>(false);
            // last_login timestamp parsing would need proper implementation
            characters.push_back(std::move(summary));
        }

        logger->debug("Loaded {} characters for user {}", characters.size(), user_id);
        return characters;

    } catch (const pqxx::sql_error& e) {
        logger->error("SQL error loading characters for user {}: {}", user_id, e.what());
        return std::unexpected(Error{ErrorCode::InternalError,
            fmt::format("Failed to load user characters: {}", e.what())});
    }
}

Result<void> update_user_last_login(pqxx::work& txn, const std::string& user_id) {
    auto logger = Log::database();
    logger->debug("Updating last login for user {}", user_id);

    try {
        txn.exec_params(R"(
            UPDATE "Users"
            SET last_login_at = NOW(),
                failed_login_attempts = 0,
                locked_until = NULL,
                updated_at = NOW()
            WHERE id = $1
        )", user_id);

        return {};

    } catch (const pqxx::sql_error& e) {
        logger->error("SQL error updating last login: {}", e.what());
        return std::unexpected(Error{ErrorCode::InternalError,
            fmt::format("Failed to update last login: {}", e.what())});
    }
}

Result<void> increment_failed_login(pqxx::work& txn, const std::string& user_id) {
    auto logger = Log::database();
    logger->debug("Incrementing failed login attempts for user {}", user_id);

    try {
        txn.exec_params(R"(
            UPDATE "Users"
            SET failed_login_attempts = failed_login_attempts + 1,
                last_failed_login = NOW(),
                updated_at = NOW()
            WHERE id = $1
        )", user_id);

        return {};

    } catch (const pqxx::sql_error& e) {
        logger->error("SQL error incrementing failed login: {}", e.what());
        return std::unexpected(Error{ErrorCode::InternalError,
            fmt::format("Failed to increment failed login: {}", e.what())});
    }
}

Result<void> lock_user_account(
    pqxx::work& txn,
    const std::string& user_id,
    std::chrono::system_clock::time_point until) {
    auto logger = Log::database();
    logger->debug("Locking user account {}", user_id);

    try {
        // Convert time_point to timestamp string using fmt
        auto timestamp = fmt::format("{:%Y-%m-%d %H:%M:%S}", until);

        txn.exec_params(R"(
            UPDATE "Users"
            SET locked_until = $2::timestamp,
                updated_at = NOW()
            WHERE id = $1
        )", user_id, timestamp);

        logger->debug("Locked user account {} until {}", user_id, timestamp);
        return {};

    } catch (const pqxx::sql_error& e) {
        logger->error("SQL error locking user account: {}", e.what());
        return std::unexpected(Error{ErrorCode::InternalError,
            fmt::format("Failed to lock user account: {}", e.what())});
    }
}

Result<void> link_character_to_user(
    pqxx::work& txn,
    const std::string& character_id,
    const std::string& user_id) {
    auto logger = Log::database();
    logger->debug("Linking character {} to user {}", character_id, user_id);

    try {
        txn.exec_params(R"(
            UPDATE "Characters"
            SET user_id = $2, updated_at = NOW()
            WHERE id = $1
        )", character_id, user_id);

        logger->debug("Linked character {} to user {}", character_id, user_id);
        return {};

    } catch (const pqxx::sql_error& e) {
        logger->error("SQL error linking character to user: {}", e.what());
        return std::unexpected(Error{ErrorCode::InternalError,
            fmt::format("Failed to link character to user: {}", e.what())});
    }
}

Result<void> save_account_wealth(
    pqxx::work& txn,
    const std::string& user_id,
    long account_wealth) {
    auto logger = Log::database();
    logger->debug("Saving account wealth {} for user {}", account_wealth, user_id);

    try {
        txn.exec_params(R"(
            UPDATE "Users"
            SET account_wealth = $2, updated_at = NOW()
            WHERE id = $1
        )", user_id, account_wealth);

        logger->debug("Saved account wealth {} for user {}", account_wealth, user_id);
        return {};

    } catch (const pqxx::sql_error& e) {
        logger->error("SQL error saving account wealth: {}", e.what());
        return std::unexpected(Error{ErrorCode::InternalError,
            fmt::format("Failed to save account wealth: {}", e.what())});
    }
}

// =============================================================================
// Shop System Queries
// =============================================================================

Result<std::vector<ShopData>> load_all_shops(pqxx::work& txn) {
    auto logger = Log::database();
    logger->debug("Loading all shops from database");

    try {
        auto result = txn.exec(R"(
            SELECT id, zone_id, keeper_zone_id, keeper_id,
                   buy_profit, sell_profit, temper,
                   no_such_item_messages, do_not_buy_messages,
                   missing_cash_messages, buy_messages, sell_messages,
                   flags::text[] AS flags, "tradesWithFlags"::text[] AS trades_with_flags
            FROM "Shops"
            WHERE keeper_zone_id IS NOT NULL AND keeper_id IS NOT NULL
            ORDER BY zone_id, id
        )");

        std::vector<ShopData> shops;
        shops.reserve(result.size());

        for (const auto& row : result) {
            ShopData shop;
            shop.zone_id = row["zone_id"].as<int>();
            shop.id = row["id"].as<int>();
            int keeper_zone = row["keeper_zone_id"].as<int>();
            int keeper_id = row["keeper_id"].as<int>();
            shop.keeper_id = EntityId(keeper_zone, keeper_id);
            shop.buy_profit = row["buy_profit"].as<float>();
            shop.sell_profit = row["sell_profit"].as<float>();
            shop.temper = row["temper"].as<int>();

            // Parse message arrays
            if (!row["no_such_item_messages"].is_null()) {
                shop.no_such_item_messages = parse_pg_array(row["no_such_item_messages"].as<std::string>());
            }
            if (!row["do_not_buy_messages"].is_null()) {
                shop.do_not_buy_messages = parse_pg_array(row["do_not_buy_messages"].as<std::string>());
            }
            if (!row["missing_cash_messages"].is_null()) {
                shop.missing_cash_messages = parse_pg_array(row["missing_cash_messages"].as<std::string>());
            }
            if (!row["buy_messages"].is_null()) {
                shop.buy_messages = parse_pg_array(row["buy_messages"].as<std::string>());
            }
            if (!row["sell_messages"].is_null()) {
                shop.sell_messages = parse_pg_array(row["sell_messages"].as<std::string>());
            }
            if (!row["flags"].is_null()) {
                shop.flags = parse_pg_array(row["flags"].as<std::string>());
            }
            if (!row["trades_with_flags"].is_null()) {
                shop.trades_with_flags = parse_pg_array(row["trades_with_flags"].as<std::string>());
            }

            shops.push_back(std::move(shop));
        }

        logger->debug("Loaded {} shops from database", shops.size());
        return shops;

    } catch (const pqxx::sql_error& e) {
        logger->error("SQL error loading shops: {}", e.what());
        return std::unexpected(Error{ErrorCode::InternalError,
            fmt::format("Failed to load shops: {}", e.what())});
    }
}

Result<std::vector<ShopData>> load_shops_in_zone(pqxx::work& txn, int zone_id) {
    auto logger = Log::database();
    logger->debug("Loading shops for zone {}", zone_id);

    try {
        auto result = txn.exec_params(R"(
            SELECT id, zone_id, keeper_zone_id, keeper_id,
                   buy_profit, sell_profit, temper,
                   no_such_item_messages, do_not_buy_messages,
                   missing_cash_messages, buy_messages, sell_messages,
                   flags::text[] AS flags, "tradesWithFlags"::text[] AS trades_with_flags
            FROM "Shops"
            WHERE zone_id = $1
              AND keeper_zone_id IS NOT NULL AND keeper_id IS NOT NULL
            ORDER BY id
        )", zone_id);

        std::vector<ShopData> shops;
        shops.reserve(result.size());

        for (const auto& row : result) {
            ShopData shop;
            shop.zone_id = row["zone_id"].as<int>();
            shop.id = row["id"].as<int>();
            int keeper_zone = row["keeper_zone_id"].as<int>();
            int keeper_id = row["keeper_id"].as<int>();
            shop.keeper_id = EntityId(keeper_zone, keeper_id);
            shop.buy_profit = row["buy_profit"].as<float>();
            shop.sell_profit = row["sell_profit"].as<float>();
            shop.temper = row["temper"].as<int>();

            // Parse message arrays
            if (!row["no_such_item_messages"].is_null()) {
                shop.no_such_item_messages = parse_pg_array(row["no_such_item_messages"].as<std::string>());
            }
            if (!row["do_not_buy_messages"].is_null()) {
                shop.do_not_buy_messages = parse_pg_array(row["do_not_buy_messages"].as<std::string>());
            }
            if (!row["missing_cash_messages"].is_null()) {
                shop.missing_cash_messages = parse_pg_array(row["missing_cash_messages"].as<std::string>());
            }
            if (!row["buy_messages"].is_null()) {
                shop.buy_messages = parse_pg_array(row["buy_messages"].as<std::string>());
            }
            if (!row["sell_messages"].is_null()) {
                shop.sell_messages = parse_pg_array(row["sell_messages"].as<std::string>());
            }
            if (!row["flags"].is_null()) {
                shop.flags = parse_pg_array(row["flags"].as<std::string>());
            }
            if (!row["trades_with_flags"].is_null()) {
                shop.trades_with_flags = parse_pg_array(row["trades_with_flags"].as<std::string>());
            }

            shops.push_back(std::move(shop));
        }

        logger->debug("Loaded {} shops for zone {}", shops.size(), zone_id);
        return shops;

    } catch (const pqxx::sql_error& e) {
        logger->error("SQL error loading shops for zone {}: {}", zone_id, e.what());
        return std::unexpected(Error{ErrorCode::InternalError,
            fmt::format("Failed to load shops for zone {}: {}", zone_id, e.what())});
    }
}

Result<std::vector<ShopItemData>> load_shop_items(
    pqxx::work& txn, int shop_zone_id, int shop_id) {
    auto logger = Log::database();
    logger->debug("Loading items for shop ({}, {})", shop_zone_id, shop_id);

    try {
        auto result = txn.exec_params(R"(
            SELECT object_zone_id, object_id, amount, price
            FROM "ShopItems"
            WHERE shop_zone_id = $1 AND shop_id = $2
            ORDER BY id
        )", shop_zone_id, shop_id);

        std::vector<ShopItemData> items;
        items.reserve(result.size());

        for (const auto& row : result) {
            ShopItemData item;
            int obj_zone = row["object_zone_id"].as<int>();
            int obj_id = row["object_id"].as<int>();
            item.object_id = EntityId(obj_zone, obj_id);
            item.amount = row["amount"].as<int>();
            item.price = row["price"].is_null() ? 0 : row["price"].as<int>();
            // Convert 0 to -1 for unlimited stock
            if (item.amount == 0) {
                item.amount = -1;
            }
            items.push_back(item);
        }

        logger->debug("Loaded {} items for shop ({}, {})", items.size(), shop_zone_id, shop_id);
        return items;

    } catch (const pqxx::sql_error& e) {
        logger->error("SQL error loading shop items: {}", e.what());
        return std::unexpected(Error{ErrorCode::InternalError,
            fmt::format("Failed to load shop items: {}", e.what())});
    }
}

Result<std::vector<ShopMobData>> load_shop_mobs(
    pqxx::work& txn, int shop_zone_id, int shop_id) {
    auto logger = Log::database();
    logger->debug("Loading mobs for shop ({}, {})", shop_zone_id, shop_id);

    try {
        auto result = txn.exec_params(R"(
            SELECT mob_zone_id, mob_id, amount, price
            FROM "ShopMobs"
            WHERE shop_zone_id = $1 AND shop_id = $2
            ORDER BY id
        )", shop_zone_id, shop_id);

        std::vector<ShopMobData> mobs;
        mobs.reserve(result.size());

        for (const auto& row : result) {
            ShopMobData mob;
            int mob_zone = row["mob_zone_id"].as<int>();
            int mob_local = row["mob_id"].as<int>();
            mob.mob_id = EntityId(mob_zone, mob_local);
            mob.amount = row["amount"].as<int>();
            mob.price = row["price"].is_null() ? 0 : row["price"].as<int>();
            // Convert 0 to -1 for unlimited stock
            if (mob.amount == 0) {
                mob.amount = -1;
            }
            mobs.push_back(mob);
        }

        logger->debug("Loaded {} mobs for shop ({}, {})", mobs.size(), shop_zone_id, shop_id);
        return mobs;

    } catch (const pqxx::sql_error& e) {
        logger->error("SQL error loading shop mobs: {}", e.what());
        return std::unexpected(Error{ErrorCode::InternalError,
            fmt::format("Failed to load shop mobs: {}", e.what())});
    }
}

Result<std::vector<ShopAbilityData>> load_shop_abilities(
    pqxx::work& txn, int shop_zone_id, int shop_id) {
    auto logger = Log::database();
    logger->debug("Loading abilities for shop ({}, {})", shop_zone_id, shop_id);

    try {
        auto result = txn.exec_params(R"(
            SELECT ability_id, price, spawn_chance,
                   visibility_requirement, purchase_requirement
            FROM "ShopAbilities"
            WHERE shop_zone_id = $1 AND shop_id = $2
            ORDER BY id
        )", shop_zone_id, shop_id);

        std::vector<ShopAbilityData> abilities;
        abilities.reserve(result.size());

        for (const auto& row : result) {
            ShopAbilityData ability;
            ability.ability_id = row["ability_id"].as<int>();
            ability.price = row["price"].is_null() ? 0 : row["price"].as<int>();
            ability.spawn_chance = row["spawn_chance"].is_null() ? 1.0f
                : row["spawn_chance"].as<float>();
            ability.visibility_requirement = row["visibility_requirement"].is_null() ? ""
                : row["visibility_requirement"].as<std::string>();
            ability.purchase_requirement = row["purchase_requirement"].is_null() ? ""
                : row["purchase_requirement"].as<std::string>();
            abilities.push_back(ability);
        }

        logger->debug("Loaded {} abilities for shop ({}, {})",
            abilities.size(), shop_zone_id, shop_id);
        return abilities;

    } catch (const pqxx::sql_error& e) {
        logger->error("SQL error loading shop abilities: {}", e.what());
        return std::unexpected(Error{ErrorCode::InternalError,
            fmt::format("Failed to load shop abilities: {}", e.what())});
    }
}

// =============================================================================
// Character Item Queries
// =============================================================================

Result<std::vector<CharacterItemData>> load_character_items(
    pqxx::work& txn, const std::string& character_id) {
    auto logger = Log::database();
    logger->debug("Loading items for character {}", character_id);

    try {
        auto result = txn.exec_params(R"(
            SELECT id, character_id, object_zone_id, object_id,
                   container_id, equipped_location, condition, charges,
                   instance_flags::text[], custom_name, custom_examine_description,
                   liquid_type, liquid_remaining, liquid_effects, liquid_identified
            FROM "CharacterItems"
            WHERE character_id = $1
            ORDER BY id
        )", character_id);

        std::vector<CharacterItemData> items;
        items.reserve(result.size());

        for (const auto& row : result) {
            CharacterItemData item;
            item.id = row["id"].as<int>();
            item.character_id = row["character_id"].as<std::string>();
            int obj_zone = row["object_zone_id"].as<int>();
            int obj_id = row["object_id"].as<int>();
            item.object_id = EntityId(obj_zone, obj_id);

            if (!row["container_id"].is_null()) {
                item.container_id = row["container_id"].as<int>();
            }

            if (!row["equipped_location"].is_null()) {
                item.equipped_location = row["equipped_location"].as<std::string>();
            }

            item.condition = row["condition"].as<int>();
            item.charges = row["charges"].as<int>();

            if (!row["instance_flags"].is_null()) {
                item.instance_flags = parse_pg_array(row["instance_flags"].as<std::string>());
            }

            if (!row["custom_name"].is_null()) {
                item.custom_name = row["custom_name"].as<std::string>();
            }

            if (!row["custom_examine_description"].is_null()) {
                item.custom_description = row["custom_examine_description"].as<std::string>();
            }

            // Load liquid container state
            if (!row["liquid_type"].is_null()) {
                item.liquid_type = row["liquid_type"].as<std::string>();
            }
            item.liquid_remaining = row["liquid_remaining"].as<int>();
            if (!row["liquid_effects"].is_null()) {
                item.liquid_effects = parse_pg_int_array(row["liquid_effects"].as<std::string>());
            }
            item.liquid_identified = row["liquid_identified"].as<bool>();

            items.push_back(std::move(item));
        }

        logger->debug("Loaded {} items for character {}", items.size(), character_id);
        return items;

    } catch (const pqxx::sql_error& e) {
        logger->error("SQL error loading character items: {}", e.what());
        return std::unexpected(Error{ErrorCode::InternalError,
            fmt::format("Failed to load character items: {}", e.what())});
    }
}

Result<void> save_character_items(
    pqxx::work& txn,
    const std::string& character_id,
    const std::vector<CharacterItemData>& items) {
    auto logger = Log::database();
    logger->debug("Saving {} items for character {}", items.size(), character_id);

    try {
        // Delete existing items for this character
        txn.exec_params(R"(
            DELETE FROM "CharacterItems" WHERE character_id = $1
        )", character_id);

        // Insert new items
        for (const auto& item : items) {
            std::optional<int> container_id_param;
            if (item.container_id) {
                container_id_param = *item.container_id;
            }

            std::string equipped_loc_param = item.equipped_location.empty()
                ? "" : item.equipped_location;

            // Build instance flags as PostgreSQL array literal (with proper escaping)
            std::string flags_array = to_pg_array(item.instance_flags);

            // Build liquid effects as PostgreSQL integer array literal
            std::string effects_array = "{}";
            if (!item.liquid_effects.empty()) {
                effects_array = "{";
                for (size_t i = 0; i < item.liquid_effects.size(); ++i) {
                    if (i > 0) effects_array += ",";
                    effects_array += std::to_string(item.liquid_effects[i]);
                }
                effects_array += "}";
            }

            txn.exec_params(R"(
                INSERT INTO "CharacterItems" (
                    character_id, object_zone_id, object_id,
                    container_id, equipped_location, condition, charges,
                    instance_flags, custom_name, custom_examine_description,
                    liquid_type, liquid_remaining, liquid_effects, liquid_identified,
                    updated_at
                ) VALUES ($1, $2, $3, $4, $5, $6, $7, $8::"ItemInstanceFlag"[], $9, $10, $11, $12, $13::int[], $14, NOW())
            )",
                character_id,
                item.object_id.zone_id(),
                item.object_id.local_id(),
                container_id_param,
                equipped_loc_param.empty() ? std::optional<std::string>{} : equipped_loc_param,
                item.condition,
                item.charges,
                flags_array,
                item.custom_name.empty() ? std::optional<std::string>{} : item.custom_name,
                item.custom_description.empty() ? std::optional<std::string>{} : item.custom_description,
                item.liquid_type.empty() ? std::optional<std::string>{} : item.liquid_type,
                item.liquid_remaining,
                effects_array,
                item.liquid_identified
            );
        }

        logger->debug("Saved {} items for character {}", items.size(), character_id);
        return Success();

    } catch (const pqxx::sql_error& e) {
        logger->error("SQL error saving character items: {}", e.what());
        return std::unexpected(Error{ErrorCode::InternalError,
            fmt::format("Failed to save character items: {}", e.what())});
    }
}

// =============================================================================
// Command Queries
// =============================================================================

Result<std::vector<CommandData>> load_all_commands(pqxx::work& txn) {
    auto logger = Log::database();
    logger->debug("Loading all commands from database");

    try {
        auto result = txn.exec(R"(
            SELECT name, aliases, category, description, usage,
                   immortal_only, permissions, ability_id
            FROM "Command"
            ORDER BY name
        )");

        std::vector<CommandData> commands;
        commands.reserve(result.size());

        for (const auto& row : result) {
            CommandData cmd;
            cmd.name = row["name"].as<std::string>();

            if (!row["aliases"].is_null()) {
                cmd.aliases = parse_pg_array(row["aliases"].as<std::string>());
            }

            if (!row["category"].is_null()) {
                cmd.category = row["category"].as<std::string>();
            }

            if (!row["description"].is_null()) {
                cmd.description = row["description"].as<std::string>();
            }

            if (!row["usage"].is_null()) {
                cmd.usage = row["usage"].as<std::string>();
            }

            cmd.immortal_only = row["immortal_only"].as<bool>();

            if (!row["permissions"].is_null()) {
                cmd.permissions = parse_pg_array(row["permissions"].as<std::string>());
            }

            if (!row["ability_id"].is_null()) {
                cmd.ability_id = row["ability_id"].as<int>();
            }

            commands.push_back(std::move(cmd));
        }

        logger->debug("Loaded {} commands from database", commands.size());
        return commands;

    } catch (const pqxx::sql_error& e) {
        logger->error("SQL error loading commands: {}", e.what());
        return std::unexpected(Error{ErrorCode::InternalError,
            fmt::format("Failed to load commands: {}", e.what())});
    }
}

Result<std::optional<CommandData>> load_command_by_name(
    pqxx::work& txn, const std::string& name) {
    auto logger = Log::database();
    logger->debug("Loading command: {}", name);

    try {
        auto result = txn.exec_params(R"(
            SELECT name, aliases, category, description, usage,
                   immortal_only, permissions, ability_id
            FROM "Command"
            WHERE LOWER(name) = LOWER($1)
        )", name);

        if (result.empty()) {
            return std::nullopt;
        }

        const auto& row = result[0];
        CommandData cmd;
        cmd.name = row["name"].as<std::string>();

        if (!row["aliases"].is_null()) {
            cmd.aliases = parse_pg_array(row["aliases"].as<std::string>());
        }

        if (!row["category"].is_null()) {
            cmd.category = row["category"].as<std::string>();
        }

        if (!row["description"].is_null()) {
            cmd.description = row["description"].as<std::string>();
        }

        if (!row["usage"].is_null()) {
            cmd.usage = row["usage"].as<std::string>();
        }

        cmd.immortal_only = row["immortal_only"].as<bool>();

        if (!row["permissions"].is_null()) {
            cmd.permissions = parse_pg_array(row["permissions"].as<std::string>());
        }

        if (!row["ability_id"].is_null()) {
            cmd.ability_id = row["ability_id"].as<int>();
        }

        return cmd;

    } catch (const pqxx::sql_error& e) {
        logger->error("SQL error loading command '{}': {}", name, e.what());
        return std::unexpected(Error{ErrorCode::InternalError,
            fmt::format("Failed to load command '{}': {}", name, e.what())});
    }
}

// =============================================================================
// Account Item Storage Queries
// =============================================================================

Result<std::vector<AccountItemData>> load_account_items(
    pqxx::work& txn, const std::string& user_id) {
    auto logger = Log::database();
    logger->debug("Loading account items for user {}", user_id);

    try {
        auto result = txn.exec_params(R"(
            SELECT id, user_id, slot, object_zone_id, object_id,
                   quantity, custom_data::text, stored_at, stored_by_character_id
            FROM "account_items"
            WHERE user_id = $1
            ORDER BY slot, id
        )", user_id);

        std::vector<AccountItemData> items;
        items.reserve(result.size());

        for (const auto& row : result) {
            AccountItemData item;
            item.id = row["id"].as<int>();
            item.user_id = row["user_id"].as<std::string>();
            item.slot = row["slot"].as<int>();

            int obj_zone = row["object_zone_id"].as<int>();
            int obj_id = row["object_id"].as<int>();
            item.object_id = EntityId(obj_zone, obj_id);

            item.quantity = row["quantity"].as<int>();

            if (!row["custom_data"].is_null()) {
                item.custom_data = row["custom_data"].as<std::string>();
            } else {
                item.custom_data = "{}";
            }

            // Parse stored_at timestamp
            if (!row["stored_at"].is_null()) {
                std::string ts_str = row["stored_at"].as<std::string>();
                // Convert PostgreSQL timestamp to time_point
                std::tm tm = {};
                std::istringstream ss(ts_str);
                ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
                item.stored_at = std::chrono::system_clock::from_time_t(std::mktime(&tm));
            } else {
                item.stored_at = std::chrono::system_clock::now();
            }

            if (!row["stored_by_character_id"].is_null()) {
                item.stored_by_character_id = row["stored_by_character_id"].as<std::string>();
            }

            items.push_back(std::move(item));
        }

        logger->debug("Loaded {} account items for user {}", items.size(), user_id);
        return items;

    } catch (const pqxx::sql_error& e) {
        logger->error("SQL error loading account items for user {}: {}", user_id, e.what());
        return std::unexpected(Error{ErrorCode::InternalError,
            fmt::format("Failed to load account items: {}", e.what())});
    }
}

Result<int> store_account_item(
    pqxx::work& txn,
    const std::string& user_id,
    EntityId object_id,
    int quantity,
    const std::optional<std::string>& character_id,
    const std::string& custom_data) {
    auto logger = Log::database();
    logger->debug("Storing item ({},{}) for user {}", object_id.zone_id(), object_id.local_id(), user_id);

    try {
        // Get the next slot number for this user
        auto slot_result = txn.exec_params(R"(
            SELECT COALESCE(MAX(slot) + 1, 0) as next_slot
            FROM "account_items"
            WHERE user_id = $1
        )", user_id);

        int next_slot = slot_result[0]["next_slot"].as<int>();

        // Insert the new item
        auto result = txn.exec_params(R"(
            INSERT INTO "account_items" (
                user_id, slot, object_zone_id, object_id,
                quantity, custom_data, stored_at, stored_by_character_id
            ) VALUES ($1, $2, $3, $4, $5, $6::json, NOW(), $7)
            RETURNING id
        )",
            user_id,
            next_slot,
            object_id.zone_id(),
            object_id.local_id(),
            quantity,
            custom_data,
            character_id ? *character_id : std::optional<std::string>{}
        );

        int new_id = result[0]["id"].as<int>();
        logger->debug("Stored account item {} for user {} (object {},{})",
            new_id, user_id, object_id.zone_id(), object_id.local_id());

        return new_id;

    } catch (const pqxx::sql_error& e) {
        logger->error("SQL error storing account item: {}", e.what());
        return std::unexpected(Error{ErrorCode::InternalError,
            fmt::format("Failed to store account item: {}", e.what())});
    }
}

Result<AccountItemData> remove_account_item(
    pqxx::work& txn, int item_id) {
    auto logger = Log::database();
    logger->debug("Removing account item {}", item_id);

    try {
        // First, fetch the item data before deleting
        auto select_result = txn.exec_params(R"(
            SELECT id, user_id, slot, object_zone_id, object_id,
                   quantity, custom_data::text, stored_at, stored_by_character_id
            FROM "account_items"
            WHERE id = $1
        )", item_id);

        if (select_result.empty()) {
            return std::unexpected(Error{ErrorCode::NotFound,
                fmt::format("Account item {} not found", item_id)});
        }

        const auto& row = select_result[0];
        AccountItemData item;
        item.id = row["id"].as<int>();
        item.user_id = row["user_id"].as<std::string>();
        item.slot = row["slot"].as<int>();

        int obj_zone = row["object_zone_id"].as<int>();
        int obj_id = row["object_id"].as<int>();
        item.object_id = EntityId(obj_zone, obj_id);

        item.quantity = row["quantity"].as<int>();

        if (!row["custom_data"].is_null()) {
            item.custom_data = row["custom_data"].as<std::string>();
        } else {
            item.custom_data = "{}";
        }

        if (!row["stored_at"].is_null()) {
            std::string ts_str = row["stored_at"].as<std::string>();
            std::tm tm = {};
            std::istringstream ss(ts_str);
            ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
            item.stored_at = std::chrono::system_clock::from_time_t(std::mktime(&tm));
        } else {
            item.stored_at = std::chrono::system_clock::now();
        }

        if (!row["stored_by_character_id"].is_null()) {
            item.stored_by_character_id = row["stored_by_character_id"].as<std::string>();
        }

        // Now delete the item
        txn.exec_params(R"(
            DELETE FROM "account_items" WHERE id = $1
        )", item_id);

        logger->debug("Removed account item {} from user {}", item_id, item.user_id);
        return item;

    } catch (const pqxx::sql_error& e) {
        logger->error("SQL error removing account item {}: {}", item_id, e.what());
        return std::unexpected(Error{ErrorCode::InternalError,
            fmt::format("Failed to remove account item: {}", e.what())});
    }
}

Result<std::optional<AccountItemData>> find_account_item(
    pqxx::work& txn,
    const std::string& user_id,
    EntityId object_id) {
    auto logger = Log::database();
    logger->debug("Finding account item ({},{}) for user {}",
        object_id.zone_id(), object_id.local_id(), user_id);

    try {
        auto result = txn.exec_params(R"(
            SELECT id, user_id, slot, object_zone_id, object_id,
                   quantity, custom_data::text, stored_at, stored_by_character_id
            FROM "account_items"
            WHERE user_id = $1 AND object_zone_id = $2 AND object_id = $3
            ORDER BY id
            LIMIT 1
        )", user_id, object_id.zone_id(), object_id.local_id());

        if (result.empty()) {
            return std::nullopt;
        }

        const auto& row = result[0];
        AccountItemData item;
        item.id = row["id"].as<int>();
        item.user_id = row["user_id"].as<std::string>();
        item.slot = row["slot"].as<int>();

        int obj_zone = row["object_zone_id"].as<int>();
        int obj_id = row["object_id"].as<int>();
        item.object_id = EntityId(obj_zone, obj_id);

        item.quantity = row["quantity"].as<int>();

        if (!row["custom_data"].is_null()) {
            item.custom_data = row["custom_data"].as<std::string>();
        } else {
            item.custom_data = "{}";
        }

        if (!row["stored_at"].is_null()) {
            std::string ts_str = row["stored_at"].as<std::string>();
            std::tm tm = {};
            std::istringstream ss(ts_str);
            ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
            item.stored_at = std::chrono::system_clock::from_time_t(std::mktime(&tm));
        } else {
            item.stored_at = std::chrono::system_clock::now();
        }

        if (!row["stored_by_character_id"].is_null()) {
            item.stored_by_character_id = row["stored_by_character_id"].as<std::string>();
        }

        return item;

    } catch (const pqxx::sql_error& e) {
        logger->error("SQL error finding account item: {}", e.what());
        return std::unexpected(Error{ErrorCode::InternalError,
            fmt::format("Failed to find account item: {}", e.what())});
    }
}

Result<int> count_account_items(
    pqxx::work& txn, const std::string& user_id) {
    auto logger = Log::database();

    try {
        auto result = txn.exec_params(R"(
            SELECT COUNT(*) as count
            FROM "account_items"
            WHERE user_id = $1
        )", user_id);

        return result[0]["count"].as<int>();

    } catch (const pqxx::sql_error& e) {
        logger->error("SQL error counting account items: {}", e.what());
        return std::unexpected(Error{ErrorCode::InternalError,
            fmt::format("Failed to count account items: {}", e.what())});
    }
}

// =============================================================================
// Player Mail Queries (Postmaster System)
// =============================================================================

namespace {

// Helper to parse PostgreSQL timestamp string to time_point
std::chrono::system_clock::time_point parse_timestamp(const std::string& timestamp_str) {
    // PostgreSQL timestamp format: "2024-01-15 10:30:45.123456" or "2024-01-15 10:30:45"
    std::tm tm = {};
    std::istringstream ss(timestamp_str);
    ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
    if (ss.fail()) {
        // Return epoch on parse failure
        return std::chrono::system_clock::time_point{};
    }
    return std::chrono::system_clock::from_time_t(std::mktime(&tm));
}

// Helper to parse a PlayerMailData from a database row
PlayerMailData parse_mail_row(const pqxx::row& row) {
    PlayerMailData mail;
    mail.id = row["id"].as<int>();

    // Legacy IDs
    if (!row["legacy_sender_id"].is_null()) {
        mail.legacy_sender_id = row["legacy_sender_id"].as<int>();
    }
    if (!row["legacy_recipient_id"].is_null()) {
        mail.legacy_recipient_id = row["legacy_recipient_id"].as<int>();
    }

    // Modern character IDs
    if (!row["sender_character_id"].is_null()) {
        mail.sender_character_id = row["sender_character_id"].as<std::string>();
    }
    if (!row["recipient_character_id"].is_null()) {
        mail.recipient_character_id = row["recipient_character_id"].as<std::string>();
    }

    // Content
    mail.body = row["body"].as<std::string>();
    mail.sent_at = parse_timestamp(row["sent_at"].as<std::string>());
    if (!row["read_at"].is_null()) {
        mail.read_at = parse_timestamp(row["read_at"].as<std::string>());
    }

    // Wealth attachments
    mail.attached_copper = row["attached_copper"].as<int>();
    mail.attached_silver = row["attached_silver"].as<int>();
    mail.attached_gold = row["attached_gold"].as<int>();
    mail.attached_platinum = row["attached_platinum"].as<int>();

    // Object attachment
    if (!row["attached_object_zone_id"].is_null() && !row["attached_object_id"].is_null()) {
        mail.attached_object_id = EntityId(
            row["attached_object_zone_id"].as<int>(),
            row["attached_object_id"].as<int>()
        );
    }

    // Retrieval tracking
    if (!row["wealth_retrieved_at"].is_null()) {
        mail.wealth_retrieved_at = parse_timestamp(row["wealth_retrieved_at"].as<std::string>());
    }
    if (!row["wealth_retrieved_by_character_id"].is_null()) {
        mail.wealth_retrieved_by_character_id = row["wealth_retrieved_by_character_id"].as<std::string>();
    }
    if (!row["object_retrieved_at"].is_null()) {
        mail.object_retrieved_at = parse_timestamp(row["object_retrieved_at"].as<std::string>());
    }
    if (!row["object_retrieved_by_character_id"].is_null()) {
        mail.object_retrieved_by_character_id = row["object_retrieved_by_character_id"].as<std::string>();
    }
    mail.object_moved_to_account_storage = row["object_moved_to_account_storage"].as<bool>();

    // Status
    mail.is_deleted = row["is_deleted"].as<bool>();

    return mail;
}
} // anonymous namespace

Result<std::vector<PlayerMailData>> load_character_mail(
    pqxx::work& txn, const std::string& character_id) {
    auto logger = Log::database();
    logger->debug("Loading mail for character {}", character_id);

    try {
        auto result = txn.exec_params(R"(
            SELECT id, legacy_sender_id, legacy_recipient_id,
                   sender_character_id, recipient_character_id,
                   body, sent_at, read_at,
                   attached_copper, attached_silver, attached_gold, attached_platinum,
                   attached_object_zone_id, attached_object_id,
                   wealth_retrieved_at, wealth_retrieved_by_character_id,
                   object_retrieved_at, object_retrieved_by_character_id,
                   object_moved_to_account_storage, is_deleted
            FROM "PlayerMail"
            WHERE recipient_character_id = $1 AND is_deleted = false
            ORDER BY sent_at DESC
        )", character_id);

        std::vector<PlayerMailData> mails;
        mails.reserve(result.size());
        for (const auto& row : result) {
            mails.push_back(parse_mail_row(row));
        }

        logger->debug("Loaded {} mail messages for character {}", mails.size(), character_id);
        return mails;

    } catch (const pqxx::sql_error& e) {
        logger->error("SQL error loading character mail: {}", e.what());
        return std::unexpected(Error{ErrorCode::InternalError,
            fmt::format("Failed to load mail: {}", e.what())});
    }
}

Result<int> count_unread_mail(
    pqxx::work& txn, const std::string& character_id) {
    auto logger = Log::database();

    try {
        auto result = txn.exec_params(R"(
            SELECT COUNT(*) as count
            FROM "PlayerMail"
            WHERE recipient_character_id = $1
              AND is_deleted = false
              AND read_at IS NULL
        )", character_id);

        return result[0]["count"].as<int>();

    } catch (const pqxx::sql_error& e) {
        logger->error("SQL error counting unread mail: {}", e.what());
        return std::unexpected(Error{ErrorCode::InternalError,
            fmt::format("Failed to count unread mail: {}", e.what())});
    }
}

Result<int> count_all_mail(
    pqxx::work& txn, const std::string& character_id) {
    auto logger = Log::database();

    try {
        auto result = txn.exec_params(R"(
            SELECT COUNT(*) as count
            FROM "PlayerMail"
            WHERE recipient_character_id = $1
              AND is_deleted = false
        )", character_id);

        return result[0]["count"].as<int>();

    } catch (const pqxx::sql_error& e) {
        logger->error("SQL error counting mail: {}", e.what());
        return std::unexpected(Error{ErrorCode::InternalError,
            fmt::format("Failed to count mail: {}", e.what())});
    }
}

Result<std::optional<PlayerMailData>> load_mail_by_id(
    pqxx::work& txn, int mail_id) {
    auto logger = Log::database();
    logger->debug("Loading mail by ID {}", mail_id);

    try {
        auto result = txn.exec_params(R"(
            SELECT id, legacy_sender_id, legacy_recipient_id,
                   sender_character_id, recipient_character_id,
                   body, sent_at, read_at,
                   attached_copper, attached_silver, attached_gold, attached_platinum,
                   attached_object_zone_id, attached_object_id,
                   wealth_retrieved_at, wealth_retrieved_by_character_id,
                   object_retrieved_at, object_retrieved_by_character_id,
                   object_moved_to_account_storage, is_deleted
            FROM "PlayerMail"
            WHERE id = $1
        )", mail_id);

        if (result.empty()) {
            return std::nullopt;
        }

        return parse_mail_row(result[0]);

    } catch (const pqxx::sql_error& e) {
        logger->error("SQL error loading mail by ID: {}", e.what());
        return std::unexpected(Error{ErrorCode::InternalError,
            fmt::format("Failed to load mail: {}", e.what())});
    }
}

Result<int> send_player_mail(
    pqxx::work& txn,
    const std::string& sender_character_id,
    const std::string& recipient_character_id,
    const std::string& body,
    int attached_copper,
    int attached_silver,
    int attached_gold,
    int attached_platinum,
    const std::optional<EntityId>& attached_object_id) {
    auto logger = Log::database();
    logger->debug("Sending mail from {} to {}", sender_character_id, recipient_character_id);

    try {
        std::optional<int> obj_zone_id;
        std::optional<int> obj_id;
        if (attached_object_id) {
            obj_zone_id = attached_object_id->zone_id();
            obj_id = attached_object_id->local_id();
        }

        auto result = txn.exec_params(R"(
            INSERT INTO "PlayerMail" (
                sender_character_id, recipient_character_id, body, sent_at,
                attached_copper, attached_silver, attached_gold, attached_platinum,
                attached_object_zone_id, attached_object_id,
                created_at, updated_at
            ) VALUES ($1, $2, $3, NOW(), $4, $5, $6, $7, $8, $9, NOW(), NOW())
            RETURNING id
        )",
            sender_character_id,
            recipient_character_id,
            body,
            attached_copper,
            attached_silver,
            attached_gold,
            attached_platinum,
            obj_zone_id ? std::optional<int>(*obj_zone_id) : std::nullopt,
            obj_id ? std::optional<int>(*obj_id) : std::nullopt
        );

        int new_id = result[0]["id"].as<int>();
        logger->debug("Mail sent successfully with ID {}", new_id);
        return new_id;

    } catch (const pqxx::sql_error& e) {
        logger->error("SQL error sending mail: {}", e.what());
        return std::unexpected(Error{ErrorCode::InternalError,
            fmt::format("Failed to send mail: {}", e.what())});
    }
}

Result<void> mark_mail_read(
    pqxx::work& txn, int mail_id) {
    auto logger = Log::database();
    logger->debug("Marking mail {} as read", mail_id);

    try {
        txn.exec_params(R"(
            UPDATE "PlayerMail"
            SET read_at = NOW()
            WHERE id = $1 AND read_at IS NULL
        )", mail_id);

        return Success();

    } catch (const pqxx::sql_error& e) {
        logger->error("SQL error marking mail read: {}", e.what());
        return std::unexpected(Error{ErrorCode::InternalError,
            fmt::format("Failed to mark mail read: {}", e.what())});
    }
}

Result<void> mark_mail_wealth_retrieved(
    pqxx::work& txn, int mail_id, const std::string& retrieved_by_character_id) {
    auto logger = Log::database();
    logger->debug("Marking wealth retrieved from mail {} by {}", mail_id, retrieved_by_character_id);

    try {
        txn.exec_params(R"(
            UPDATE "PlayerMail"
            SET wealth_retrieved_at = NOW(),
                wealth_retrieved_by_character_id = $2
            WHERE id = $1 AND wealth_retrieved_at IS NULL
        )", mail_id, retrieved_by_character_id);

        return Success();

    } catch (const pqxx::sql_error& e) {
        logger->error("SQL error marking wealth retrieved: {}", e.what());
        return std::unexpected(Error{ErrorCode::InternalError,
            fmt::format("Failed to mark wealth retrieved: {}", e.what())});
    }
}

Result<void> mark_mail_object_retrieved(
    pqxx::work& txn, int mail_id, const std::string& retrieved_by_character_id,
    bool moved_to_account_storage) {
    auto logger = Log::database();
    logger->debug("Marking object retrieved from mail {} by {} (to_account: {})",
        mail_id, retrieved_by_character_id, moved_to_account_storage);

    try {
        txn.exec_params(R"(
            UPDATE "PlayerMail"
            SET object_retrieved_at = NOW(),
                object_retrieved_by_character_id = $2,
                object_moved_to_account_storage = $3
            WHERE id = $1 AND object_retrieved_at IS NULL
        )", mail_id, retrieved_by_character_id, moved_to_account_storage);

        return Success();

    } catch (const pqxx::sql_error& e) {
        logger->error("SQL error marking object retrieved: {}", e.what());
        return std::unexpected(Error{ErrorCode::InternalError,
            fmt::format("Failed to mark object retrieved: {}", e.what())});
    }
}

Result<void> delete_mail(
    pqxx::work& txn, int mail_id) {
    auto logger = Log::database();
    logger->debug("Soft-deleting mail {}", mail_id);

    try {
        txn.exec_params(R"(
            UPDATE "PlayerMail"
            SET is_deleted = true
            WHERE id = $1
        )", mail_id);

        return Success();

    } catch (const pqxx::sql_error& e) {
        logger->error("SQL error deleting mail: {}", e.what());
        return std::unexpected(Error{ErrorCode::InternalError,
            fmt::format("Failed to delete mail: {}", e.what())});
    }
}

Result<std::optional<std::string>> find_character_id_by_name(
    pqxx::work& txn, const std::string& name) {
    auto logger = Log::database();
    logger->debug("Finding character by name: {}", name);

    try {
        auto result = txn.exec_params(R"(
            SELECT id FROM "Characters"
            WHERE LOWER(name) = LOWER($1)
            LIMIT 1
        )", name);

        if (result.empty()) {
            return std::nullopt;
        }

        return result[0]["id"].as<std::string>();

    } catch (const pqxx::sql_error& e) {
        logger->error("SQL error finding character by name: {}", e.what());
        return std::unexpected(Error{ErrorCode::InternalError,
            fmt::format("Failed to find character: {}", e.what())});
    }
}

Result<std::optional<std::string>> get_character_name_by_id(
    pqxx::work& txn, const std::string& character_id) {
    auto logger = Log::database();

    try {
        auto result = txn.exec_params(R"(
            SELECT name FROM "Characters"
            WHERE id = $1
            LIMIT 1
        )", character_id);

        if (result.empty()) {
            return std::nullopt;
        }

        return result[0]["name"].as<std::string>();

    } catch (const pqxx::sql_error& e) {
        logger->error("SQL error getting character name: {}", e.what());
        return std::unexpected(Error{ErrorCode::InternalError,
            fmt::format("Failed to get character name: {}", e.what())});
    }
}

// =============================================================================
// System Text Queries (MOTD, Credits, News, Policy)
// =============================================================================

Result<std::optional<SystemTextData>> load_system_text(
    pqxx::work& txn, const std::string& key) {
    auto logger = Log::database();
    logger->debug("Loading system text: {}", key);

    try {
        auto result = txn.exec_params(R"(
            SELECT id, key, category, title, content, min_level, is_active
            FROM "SystemText"
            WHERE key = $1 AND is_active = true
            LIMIT 1
        )", key);

        if (result.empty()) {
            return std::nullopt;
        }

        const auto& row = result[0];
        SystemTextData data;
        data.id = row["id"].as<int>();
        data.key = row["key"].as<std::string>();
        data.category = row["category"].as<std::string>();
        if (!row["title"].is_null()) {
            data.title = row["title"].as<std::string>();
        }
        data.content = row["content"].as<std::string>();
        data.min_level = row["min_level"].as<int>();
        data.is_active = row["is_active"].as<bool>();

        return data;

    } catch (const pqxx::sql_error& e) {
        logger->error("SQL error loading system text '{}': {}", key, e.what());
        return std::unexpected(Error{ErrorCode::InternalError,
            fmt::format("Failed to load system text '{}': {}", key, e.what())});
    }
}

Result<std::vector<SystemTextData>> load_all_system_text(pqxx::work& txn) {
    auto logger = Log::database();
    logger->debug("Loading all system text entries");

    try {
        auto result = txn.exec(R"(
            SELECT id, key, category, title, content, min_level, is_active
            FROM "SystemText"
            WHERE is_active = true
            ORDER BY category, key
        )");

        std::vector<SystemTextData> entries;
        entries.reserve(result.size());

        for (const auto& row : result) {
            SystemTextData data;
            data.id = row["id"].as<int>();
            data.key = row["key"].as<std::string>();
            data.category = row["category"].as<std::string>();
            if (!row["title"].is_null()) {
                data.title = row["title"].as<std::string>();
            }
            data.content = row["content"].as<std::string>();
            data.min_level = row["min_level"].as<int>();
            data.is_active = row["is_active"].as<bool>();
            entries.push_back(std::move(data));
        }

        logger->debug("Loaded {} system text entries", entries.size());
        return entries;

    } catch (const pqxx::sql_error& e) {
        logger->error("SQL error loading system text: {}", e.what());
        return std::unexpected(Error{ErrorCode::InternalError,
            fmt::format("Failed to load system text: {}", e.what())});
    }
}

// =============================================================================
// Help Entry Queries
// =============================================================================

Result<std::optional<HelpEntryData>> load_help_entry(
    pqxx::work& txn, const std::string& keyword) {
    auto logger = Log::database();
    logger->debug("Searching help for keyword: {}", keyword);

    try {
        // Search for exact match first, then partial match
        // Uses PostgreSQL array containment with case-insensitive comparison
        auto result = txn.exec_params(R"(
            SELECT id, keywords, title, content, min_level, category,
                   usage, duration, sphere
            FROM "HelpEntry"
            WHERE EXISTS (
                SELECT 1 FROM unnest(keywords) AS kw
                WHERE LOWER(kw) = LOWER($1)
            )
            LIMIT 1
        )", keyword);

        // If no exact match, try partial match
        if (result.empty()) {
            result = txn.exec_params(R"(
                SELECT id, keywords, title, content, min_level, category,
                       usage, duration, sphere
                FROM "HelpEntry"
                WHERE EXISTS (
                    SELECT 1 FROM unnest(keywords) AS kw
                    WHERE LOWER(kw) LIKE LOWER($1) || '%'
                )
                ORDER BY title
                LIMIT 1
            )", keyword);
        }

        if (result.empty()) {
            return std::nullopt;
        }

        const auto& row = result[0];
        HelpEntryData data;
        data.id = row["id"].as<int>();
        if (!row["keywords"].is_null()) {
            data.keywords = parse_pg_array(row["keywords"].as<std::string>());
        }
        data.title = row["title"].as<std::string>();
        data.content = row["content"].as<std::string>();
        data.min_level = row["min_level"].as<int>();

        if (!row["category"].is_null()) {
            data.category = row["category"].as<std::string>();
        }
        if (!row["usage"].is_null()) {
            data.usage = row["usage"].as<std::string>();
        }
        if (!row["duration"].is_null()) {
            data.duration = row["duration"].as<std::string>();
        }
        if (!row["sphere"].is_null()) {
            data.sphere = row["sphere"].as<std::string>();
        }

        return data;

    } catch (const pqxx::sql_error& e) {
        logger->error("SQL error searching help for '{}': {}", keyword, e.what());
        return std::unexpected(Error{ErrorCode::InternalError,
            fmt::format("Failed to search help: {}", e.what())});
    }
}

Result<std::vector<HelpEntryData>> load_all_help_entries(pqxx::work& txn) {
    auto logger = Log::database();
    logger->debug("Loading all help entries");

    try {
        auto result = txn.exec(R"(
            SELECT id, keywords, title, content, min_level, category,
                   usage, duration, sphere
            FROM "HelpEntry"
            ORDER BY title
        )");

        std::vector<HelpEntryData> entries;
        entries.reserve(result.size());

        for (const auto& row : result) {
            HelpEntryData data;
            data.id = row["id"].as<int>();
            if (!row["keywords"].is_null()) {
                data.keywords = parse_pg_array(row["keywords"].as<std::string>());
            }
            data.title = row["title"].as<std::string>();
            data.content = row["content"].as<std::string>();
            data.min_level = row["min_level"].as<int>();

            if (!row["category"].is_null()) {
                data.category = row["category"].as<std::string>();
            }
            if (!row["usage"].is_null()) {
                data.usage = row["usage"].as<std::string>();
            }
            if (!row["duration"].is_null()) {
                data.duration = row["duration"].as<std::string>();
            }
            if (!row["sphere"].is_null()) {
                data.sphere = row["sphere"].as<std::string>();
            }

            entries.push_back(std::move(data));
        }

        logger->debug("Loaded {} help entries", entries.size());
        return entries;

    } catch (const pqxx::sql_error& e) {
        logger->error("SQL error loading help entries: {}", e.what());
        return std::unexpected(Error{ErrorCode::InternalError,
            fmt::format("Failed to load help entries: {}", e.what())});
    }
}

// =============================================================================
// Player Toggle Queries
// =============================================================================

Result<std::vector<PlayerToggleData>> load_all_player_toggles(pqxx::work& txn) {
    auto logger = Log::database();
    logger->debug("Loading all player toggle definitions");

    try {
        auto result = txn.exec(R"(
            SELECT id, name, display_name, description, default_value,
                   min_level, category, sort_order
            FROM "PlayerToggle"
            ORDER BY sort_order, category, name
        )");

        std::vector<PlayerToggleData> toggles;
        toggles.reserve(result.size());

        for (const auto& row : result) {
            PlayerToggleData data;
            data.id = row["id"].as<int>();
            data.name = row["name"].as<std::string>();
            data.display_name = row["display_name"].as<std::string>();
            data.description = row["description"].as<std::string>();
            data.default_value = row["default_value"].as<bool>();
            data.min_level = row["min_level"].as<int>();
            data.category = row["category"].as<std::string>();
            data.sort_order = row["sort_order"].as<int>();
            toggles.push_back(std::move(data));
        }

        logger->debug("Loaded {} player toggle definitions", toggles.size());
        return toggles;

    } catch (const pqxx::sql_error& e) {
        logger->error("SQL error loading player toggles: {}", e.what());
        return std::unexpected(Error{ErrorCode::InternalError,
            fmt::format("Failed to load player toggles: {}", e.what())});
    }
}

Result<std::optional<PlayerToggleData>> load_player_toggle(
    pqxx::work& txn, const std::string& name) {
    auto logger = Log::database();
    logger->debug("Loading player toggle: {}", name);

    try {
        auto result = txn.exec_params(R"(
            SELECT id, name, display_name, description, default_value,
                   min_level, category, sort_order
            FROM "PlayerToggle"
            WHERE LOWER(name) = LOWER($1)
            LIMIT 1
        )", name);

        if (result.empty()) {
            return std::nullopt;
        }

        const auto& row = result[0];
        PlayerToggleData data;
        data.id = row["id"].as<int>();
        data.name = row["name"].as<std::string>();
        data.display_name = row["display_name"].as<std::string>();
        data.description = row["description"].as<std::string>();
        data.default_value = row["default_value"].as<bool>();
        data.min_level = row["min_level"].as<int>();
        data.category = row["category"].as<std::string>();
        data.sort_order = row["sort_order"].as<int>();

        return data;

    } catch (const pqxx::sql_error& e) {
        logger->error("SQL error loading player toggle '{}': {}", name, e.what());
        return std::unexpected(Error{ErrorCode::InternalError,
            fmt::format("Failed to load player toggle '{}': {}", name, e.what())});
    }
}

// =============================================================================
// Board System Queries
// =============================================================================

Result<std::vector<BoardMessageEditData>> load_message_edits(
    pqxx::work& txn, int message_id) {
    auto logger = Log::database();

    try {
        auto result = txn.exec_params(R"(
            SELECT id, message_id, editor, edited_at
            FROM "BoardMessageEdit"
            WHERE message_id = $1
            ORDER BY edited_at ASC
        )", message_id);

        std::vector<BoardMessageEditData> edits;
        edits.reserve(result.size());

        for (const auto& row : result) {
            BoardMessageEditData edit;
            edit.id = row["id"].as<int>();
            edit.message_id = row["message_id"].as<int>();
            edit.editor = row["editor"].as<std::string>();

            // Parse timestamp
            std::string timestamp_str = row["edited_at"].as<std::string>();
            std::tm tm = {};
            std::istringstream ss(timestamp_str);
            ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
            edit.edited_at = std::chrono::system_clock::from_time_t(std::mktime(&tm));

            edits.push_back(std::move(edit));
        }

        return edits;

    } catch (const pqxx::sql_error& e) {
        logger->error("SQL error loading message edits for message {}: {}",
                     message_id, e.what());
        return std::unexpected(Error{ErrorCode::InternalError,
            fmt::format("Failed to load message edits: {}", e.what())});
    }
}

Result<std::vector<BoardMessageData>> load_board_messages(
    pqxx::work& txn, int board_id) {
    auto logger = Log::database();

    try {
        auto result = txn.exec_params(R"(
            SELECT id, board_id, poster, poster_level, posted_at,
                   subject, content, sticky
            FROM "BoardMessage"
            WHERE board_id = $1
            ORDER BY sticky DESC, posted_at DESC
        )", board_id);

        std::vector<BoardMessageData> messages;
        messages.reserve(result.size());

        for (const auto& row : result) {
            BoardMessageData msg;
            msg.id = row["id"].as<int>();
            msg.board_id = row["board_id"].as<int>();
            msg.poster = row["poster"].as<std::string>();
            msg.poster_level = row["poster_level"].as<int>();

            // Parse timestamp
            std::string timestamp_str = row["posted_at"].as<std::string>();
            std::tm tm = {};
            std::istringstream ss(timestamp_str);
            ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
            msg.posted_at = std::chrono::system_clock::from_time_t(std::mktime(&tm));

            msg.subject = row["subject"].as<std::string>();
            msg.content = row["content"].as<std::string>();
            msg.sticky = row["sticky"].as<bool>();

            // Load edit history for this message
            auto edits_result = load_message_edits(txn, msg.id);
            if (edits_result) {
                msg.edits = std::move(*edits_result);
            }

            messages.push_back(std::move(msg));
        }

        logger->debug("Loaded {} messages for board {}", messages.size(), board_id);
        return messages;

    } catch (const pqxx::sql_error& e) {
        logger->error("SQL error loading messages for board {}: {}",
                     board_id, e.what());
        return std::unexpected(Error{ErrorCode::InternalError,
            fmt::format("Failed to load board messages: {}", e.what())});
    }
}

Result<std::vector<BoardDataDB>> load_all_boards(pqxx::work& txn) {
    auto logger = Log::database();
    logger->debug("Loading all boards from database");

    try {
        auto result = txn.exec(R"(
            SELECT id, alias, title, locked, privileges
            FROM "Board"
            ORDER BY id
        )");

        std::vector<BoardDataDB> boards;
        boards.reserve(result.size());

        for (const auto& row : result) {
            BoardDataDB board;
            board.id = row["id"].as<int>();
            board.alias = row["alias"].as<std::string>();
            board.title = row["title"].as<std::string>();
            board.locked = row["locked"].as<bool>();

            if (!row["privileges"].is_null()) {
                board.privileges = row["privileges"].as<std::string>();
            } else {
                board.privileges = "[]";
            }

            // Load messages for this board
            auto messages_result = load_board_messages(txn, board.id);
            if (messages_result) {
                board.messages = std::move(*messages_result);
            }

            boards.push_back(std::move(board));
        }

        logger->debug("Loaded {} boards from database", boards.size());
        return boards;

    } catch (const pqxx::sql_error& e) {
        logger->error("SQL error loading boards: {}", e.what());
        return std::unexpected(Error{ErrorCode::InternalError,
            fmt::format("Failed to load boards: {}", e.what())});
    }
}

Result<std::optional<BoardDataDB>> load_board(pqxx::work& txn, int board_id) {
    auto logger = Log::database();
    logger->debug("Loading board {}", board_id);

    try {
        auto result = txn.exec_params(R"(
            SELECT id, alias, title, locked, privileges
            FROM "Board"
            WHERE id = $1
            LIMIT 1
        )", board_id);

        if (result.empty()) {
            return std::nullopt;
        }

        const auto& row = result[0];
        BoardDataDB board;
        board.id = row["id"].as<int>();
        board.alias = row["alias"].as<std::string>();
        board.title = row["title"].as<std::string>();
        board.locked = row["locked"].as<bool>();

        if (!row["privileges"].is_null()) {
            board.privileges = row["privileges"].as<std::string>();
        } else {
            board.privileges = "[]";
        }

        // Load messages for this board
        auto messages_result = load_board_messages(txn, board.id);
        if (messages_result) {
            board.messages = std::move(*messages_result);
        }

        return board;

    } catch (const pqxx::sql_error& e) {
        logger->error("SQL error loading board {}: {}", board_id, e.what());
        return std::unexpected(Error{ErrorCode::InternalError,
            fmt::format("Failed to load board {}: {}", board_id, e.what())});
    }
}

Result<std::optional<BoardDataDB>> load_board_by_alias(
    pqxx::work& txn, const std::string& alias) {
    auto logger = Log::database();
    logger->debug("Loading board by alias: {}", alias);

    try {
        auto result = txn.exec_params(R"(
            SELECT id, alias, title, locked, privileges
            FROM "Board"
            WHERE LOWER(alias) = LOWER($1)
            LIMIT 1
        )", alias);

        if (result.empty()) {
            return std::nullopt;
        }

        const auto& row = result[0];
        BoardDataDB board;
        board.id = row["id"].as<int>();
        board.alias = row["alias"].as<std::string>();
        board.title = row["title"].as<std::string>();
        board.locked = row["locked"].as<bool>();

        if (!row["privileges"].is_null()) {
            board.privileges = row["privileges"].as<std::string>();
        } else {
            board.privileges = "[]";
        }

        // Load messages for this board
        auto messages_result = load_board_messages(txn, board.id);
        if (messages_result) {
            board.messages = std::move(*messages_result);
        }

        return board;

    } catch (const pqxx::sql_error& e) {
        logger->error("SQL error loading board '{}': {}", alias, e.what());
        return std::unexpected(Error{ErrorCode::InternalError,
            fmt::format("Failed to load board '{}': {}", alias, e.what())});
    }
}

Result<int> count_boards(pqxx::work& txn) {
    auto logger = Log::database();

    try {
        auto result = txn.exec(R"(SELECT COUNT(*) as count FROM "Board")");
        return result[0]["count"].as<int>();

    } catch (const pqxx::sql_error& e) {
        logger->error("SQL error counting boards: {}", e.what());
        return std::unexpected(Error{ErrorCode::InternalError,
            fmt::format("Failed to count boards: {}", e.what())});
    }
}

Result<int> post_board_message(
    pqxx::work& txn, int board_id, const std::string& poster,
    int poster_level, const std::string& subject, const std::string& content) {

    auto logger = Log::database();

    try {
        auto result = txn.exec_params(R"(
            INSERT INTO "BoardMessage" (board_id, poster, poster_level, posted_at, subject, content, updated_at)
            VALUES ($1, $2, $3, NOW(), $4, $5, NOW())
            RETURNING id
        )", board_id, poster, poster_level, subject, content);

        if (result.empty()) {
            return std::unexpected(Error{ErrorCode::InternalError,
                "Failed to insert board message"});
        }

        int message_id = result[0]["id"].as<int>();
        logger->debug("Posted message {} to board {} by {}", message_id, board_id, poster);
        return message_id;

    } catch (const pqxx::sql_error& e) {
        logger->error("SQL error posting board message: {}", e.what());
        return std::unexpected(Error{ErrorCode::InternalError,
            fmt::format("Failed to post message: {}", e.what())});
    }
}

Result<bool> delete_board_message(pqxx::work& txn, int message_id) {
    auto logger = Log::database();

    try {
        auto result = txn.exec_params(R"(
            DELETE FROM "BoardMessage" WHERE id = $1
        )", message_id);

        bool deleted = result.affected_rows() > 0;
        if (deleted) {
            logger->debug("Deleted board message {}", message_id);
        }
        return deleted;

    } catch (const pqxx::sql_error& e) {
        logger->error("SQL error deleting board message: {}", e.what());
        return std::unexpected(Error{ErrorCode::InternalError,
            fmt::format("Failed to delete message: {}", e.what())});
    }
}

// =============================================================================
// Report System Queries
// =============================================================================

Result<int> save_report(
    pqxx::work& txn,
    const std::string& report_type,
    const std::string& reporter_name,
    const std::optional<std::string>& reporter_id,
    const std::optional<int>& room_zone_id,
    const std::optional<int>& room_id,
    const std::string& message) {

    auto logger = Log::database();
    logger->debug("Saving {} report from {}", report_type, reporter_name);

    try {
        // Build the query with optional fields
        auto result = txn.exec_params(R"(
            INSERT INTO "reports" (
                report_type, reporter_name, reporter_id,
                room_zone_id, room_id, message,
                status, created_at, updated_at
            ) VALUES (
                $1::text::"ReportType", $2, $3, $4, $5, $6,
                'OPEN'::"ReportStatus", NOW(), NOW()
            )
            RETURNING id
        )",
            report_type,
            reporter_name,
            reporter_id.has_value() ? reporter_id.value() : std::optional<std::string>{},
            room_zone_id.has_value() ? room_zone_id.value() : std::optional<int>{},
            room_id.has_value() ? room_id.value() : std::optional<int>{},
            message);

        if (result.empty()) {
            return std::unexpected(Error{ErrorCode::InternalError,
                "Failed to insert report - no ID returned"});
        }

        int report_id = result[0]["id"].as<int>();
        logger->info("Saved {} report #{} from {}", report_type, report_id, reporter_name);
        return report_id;

    } catch (const pqxx::sql_error& e) {
        logger->error("SQL error saving report: {}", e.what());
        return std::unexpected(Error{ErrorCode::InternalError,
            fmt::format("Failed to save report: {}", e.what())});
    }
}

Result<std::vector<ReportData>> load_reports_by_type(
    pqxx::work& txn, const std::string& report_type) {

    auto logger = Log::database();

    try {
        auto result = txn.exec_params(R"(
            SELECT id, report_type::text, status::text, reporter_name, reporter_id,
                   room_zone_id, room_id, message, resolved_by, resolved_at,
                   resolution, created_at, updated_at
            FROM "reports"
            WHERE report_type = $1::text::"ReportType"
            ORDER BY created_at DESC
        )", report_type);

        std::vector<ReportData> reports;
        reports.reserve(result.size());

        for (const auto& row : result) {
            ReportData report;
            report.id = row["id"].as<int>();
            report.report_type = row["report_type"].as<std::string>();
            report.status = row["status"].as<std::string>();
            report.reporter_name = row["reporter_name"].as<std::string>();

            if (!row["reporter_id"].is_null()) {
                report.reporter_id = row["reporter_id"].as<std::string>();
            }
            if (!row["room_zone_id"].is_null()) {
                report.room_zone_id = row["room_zone_id"].as<int>();
            }
            if (!row["room_id"].is_null()) {
                report.room_id = row["room_id"].as<int>();
            }

            report.message = row["message"].as<std::string>();

            if (!row["resolved_by"].is_null()) {
                report.resolved_by = row["resolved_by"].as<std::string>();
            }
            if (!row["resolution"].is_null()) {
                report.resolution = row["resolution"].as<std::string>();
            }

            reports.push_back(std::move(report));
        }

        logger->debug("Loaded {} {} reports", reports.size(), report_type);
        return reports;

    } catch (const pqxx::sql_error& e) {
        logger->error("SQL error loading reports: {}", e.what());
        return std::unexpected(Error{ErrorCode::InternalError,
            fmt::format("Failed to load reports: {}", e.what())});
    }
}

Result<std::vector<ReportData>> load_open_reports(
    pqxx::work& txn, const std::string& report_type) {

    auto logger = Log::database();

    try {
        auto result = txn.exec_params(R"(
            SELECT id, report_type::text, status::text, reporter_name, reporter_id,
                   room_zone_id, room_id, message, resolved_by, resolved_at,
                   resolution, created_at, updated_at
            FROM "reports"
            WHERE report_type = $1::text::"ReportType"
              AND status = 'OPEN'::"ReportStatus"
            ORDER BY created_at DESC
        )", report_type);

        std::vector<ReportData> reports;
        reports.reserve(result.size());

        for (const auto& row : result) {
            ReportData report;
            report.id = row["id"].as<int>();
            report.report_type = row["report_type"].as<std::string>();
            report.status = row["status"].as<std::string>();
            report.reporter_name = row["reporter_name"].as<std::string>();

            if (!row["reporter_id"].is_null()) {
                report.reporter_id = row["reporter_id"].as<std::string>();
            }
            if (!row["room_zone_id"].is_null()) {
                report.room_zone_id = row["room_zone_id"].as<int>();
            }
            if (!row["room_id"].is_null()) {
                report.room_id = row["room_id"].as<int>();
            }

            report.message = row["message"].as<std::string>();

            reports.push_back(std::move(report));
        }

        logger->debug("Loaded {} open {} reports", reports.size(), report_type);
        return reports;

    } catch (const pqxx::sql_error& e) {
        logger->error("SQL error loading open reports: {}", e.what());
        return std::unexpected(Error{ErrorCode::InternalError,
            fmt::format("Failed to load reports: {}", e.what())});
    }
}

Result<int> count_open_reports(pqxx::work& txn, const std::string& report_type) {
    auto logger = Log::database();

    try {
        auto result = txn.exec_params(R"(
            SELECT COUNT(*) as count
            FROM "reports"
            WHERE report_type = $1::text::"ReportType"
              AND status = 'OPEN'::"ReportStatus"
        )", report_type);

        return result[0]["count"].as<int>();

    } catch (const pqxx::sql_error& e) {
        logger->error("SQL error counting reports: {}", e.what());
        return std::unexpected(Error{ErrorCode::InternalError,
            fmt::format("Failed to count reports: {}", e.what())});
    }
}

// ============================================================================
// Character Aliases
// ============================================================================

Result<std::vector<CharacterAliasData>> load_character_aliases(
    pqxx::work& txn, const std::string& character_id) {

    auto logger = Log::database();
    logger->debug("Loading aliases for character {}", character_id);

    try {
        auto result = txn.exec_params(R"(
            SELECT alias, command
            FROM "CharacterAliases"
            WHERE character_id = $1
            ORDER BY alias
        )", character_id);

        std::vector<CharacterAliasData> aliases;
        aliases.reserve(result.size());

        for (const auto& row : result) {
            CharacterAliasData alias;
            alias.character_id = character_id;
            alias.alias = row["alias"].as<std::string>();
            alias.command = row["command"].as<std::string>();
            aliases.push_back(std::move(alias));
        }

        logger->debug("Loaded {} aliases for character {}", aliases.size(), character_id);
        return aliases;

    } catch (const pqxx::sql_error& e) {
        logger->error("SQL error loading aliases for character {}: {}", character_id, e.what());
        return std::unexpected(Error{ErrorCode::InternalError,
            fmt::format("Failed to load aliases: {}", e.what())});
    }
}

Result<void> save_character_aliases(
    pqxx::work& txn,
    const std::string& character_id,
    const std::vector<CharacterAliasData>& aliases) {

    auto logger = Log::database();
    logger->debug("Saving {} aliases for character {}", aliases.size(), character_id);

    try {
        // Delete existing aliases first
        txn.exec_params(R"(
            DELETE FROM "CharacterAliases"
            WHERE character_id = $1
        )", character_id);

        // Insert new aliases
        for (const auto& alias : aliases) {
            txn.exec_params(R"(
                INSERT INTO "CharacterAliases" (character_id, alias, command)
                VALUES ($1, $2, $3)
            )", character_id, alias.alias, alias.command);
        }

        logger->debug("Saved {} aliases for character {}", aliases.size(), character_id);
        return Success();

    } catch (const pqxx::sql_error& e) {
        logger->error("SQL error saving aliases for character {}: {}", character_id, e.what());
        return std::unexpected(Error{ErrorCode::InternalError,
            fmt::format("Failed to save aliases: {}", e.what())});
    }
}

Result<void> delete_character_aliases(
    pqxx::work& txn, const std::string& character_id) {

    auto logger = Log::database();
    logger->debug("Deleting aliases for character {}", character_id);

    try {
        txn.exec_params(R"(
            DELETE FROM "CharacterAliases"
            WHERE character_id = $1
        )", character_id);

        logger->debug("Deleted aliases for character {}", character_id);
        return Success();

    } catch (const pqxx::sql_error& e) {
        logger->error("SQL error deleting aliases for character {}: {}", character_id, e.what());
        return std::unexpected(Error{ErrorCode::InternalError,
            fmt::format("Failed to delete aliases: {}", e.what())});
    }
}

} // namespace WorldQueries
