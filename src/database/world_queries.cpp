#include "database/world_queries.hpp"
#include "core/logging.hpp"
#include "text/string_utils.hpp"
#include <fmt/format.h>
#include <sstream>
#include <regex>
#include <random>
#include <functional>
#include <algorithm>
#include <crypt.h>
#include <openssl/rand.h>
#include <openssl/evp.h>
#include <cstring>

namespace WorldQueries {

// Helper function to parse PostgreSQL array format: {elem1,elem2,elem3} or {"elem 1","elem 2"}
static std::vector<std::string> parse_pg_array(const std::string& pg_array) {
    std::vector<std::string> result;

    if (pg_array.empty() || pg_array == "{}") {
        return result;
    }

    // Remove outer braces
    std::string content = pg_array;
    if (content.front() == '{' && content.back() == '}') {
        content = content.substr(1, content.length() - 2);
    }

    if (content.empty()) {
        return result;
    }

    // Parse comma-separated values, handling quoted strings
    std::string current;
    bool in_quotes = false;
    bool escape_next = false;

    for (size_t i = 0; i < content.size(); ++i) {
        char c = content[i];

        if (escape_next) {
            current += c;
            escape_next = false;
            continue;
        }

        if (c == '\\') {
            escape_next = true;
            continue;
        }

        if (c == '"') {
            in_quotes = !in_quotes;
            continue;
        }

        if (c == ',' && !in_quotes) {
            if (!current.empty()) {
                result.push_back(current);
                current.clear();
            }
            continue;
        }

        current += c;
    }

    // Don't forget the last element
    if (!current.empty()) {
        result.push_back(current);
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

// Helper function to map database RoomFlag enum to C++ RoomFlag
static std::optional<RoomFlag> room_flag_from_db_string(const std::string& flag_str) {
    static const std::unordered_map<std::string, RoomFlag> flag_map = {
        {"DARK", RoomFlag::Dark},
        {"DEATH", RoomFlag::Death},
        {"NO_MOB", RoomFlag::NoMob},
        {"INDOORS", RoomFlag::Indoors},
        {"PEACEFUL", RoomFlag::Peaceful},
        {"SOUNDPROOF", RoomFlag::Soundproof},
        {"NO_TRACK", RoomFlag::NoTrack},
        {"NO_MAGIC", RoomFlag::NoMagic},
        {"TUNNEL", RoomFlag::Tunnel},
        {"PRIVATE", RoomFlag::Private},
        {"GODROOM", RoomFlag::Godroom},
        {"HOUSE", RoomFlag::House},
        {"HOUSE_CRASH", RoomFlag::HouseCrash},
        {"ATRIUM", RoomFlag::Atrium},
        {"OLC", RoomFlag::OLC},
        {"BFS_MARK", RoomFlag::BFS_Mark},
        {"VEHICLE", RoomFlag::Vehicle},
        {"UNDERGROUND", RoomFlag::Underground},
        {"CURRENT", RoomFlag::Current},
        {"TIMED_DT", RoomFlag::Timed_DT},
        {"EARTH_SECT", RoomFlag::Earth},
        {"AIR_SECT", RoomFlag::Air},
        {"FIRE_SECT", RoomFlag::Fire},
        {"WATER_SECT", RoomFlag::Water},
        {"NO_RECALL", RoomFlag::NoRecall},
        {"NO_SUMMON", RoomFlag::NoSummon},
        {"NO_LOCATE", RoomFlag::NoLocate},
        {"NO_TELEPORT", RoomFlag::NoTeleport},
        {"ALWAYS_LIT", RoomFlag::AlwaysLit},
        {"ARENA", RoomFlag::Arena},
        {"SHOP", RoomFlag::Shop},
        {"TEMPLE", RoomFlag::Temple},
        {"BANK", RoomFlag::Bank},
    };

    auto it = flag_map.find(flag_str);
    if (it != flag_map.end()) {
        return it->second;
    }
    return std::nullopt;
}

// Helper function to map database Direction enum to C++ Direction
static std::optional<Direction> direction_from_db_string(const std::string& dir_str) {
    static const std::unordered_map<std::string, Direction> dir_map = {
        {"NORTH", Direction::North},
        {"EAST", Direction::East},
        {"SOUTH", Direction::South},
        {"WEST", Direction::West},
        {"UP", Direction::Up},
        {"DOWN", Direction::Down},
    };

    auto it = dir_map.find(dir_str);
    if (it != dir_map.end()) {
        return it->second;
    }
    return std::nullopt;
}

Result<std::vector<std::unique_ptr<Zone>>> load_all_zones(pqxx::work& txn) {
    auto logger = Log::database();
    logger->debug("Loading all zones from database");

    try {
        auto result = txn.exec(R"(
            SELECT id, name, lifespan, "resetMode"
            FROM "Zones"
            ORDER BY id
        )");

        std::vector<std::unique_ptr<Zone>> zones;
        zones.reserve(result.size());

        for (const auto& row : result) {
            int zone_id = row["id"].as<int>();
            std::string name = row["name"].as<std::string>();
            int lifespan = row["lifespan"].as<int>(30);

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

        logger->info("Loaded {} zones from database", zones.size());
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
        auto result = txn.exec_params(R"(
            SELECT id, name, lifespan, "resetMode"
            FROM "Zones"
            WHERE id = $1
        )", zone_id);

        if (result.empty()) {
            return std::unexpected(Error{ErrorCode::NotFound,
                fmt::format("Zone {} not found", zone_id)});
        }

        const auto& row = result[0];
        std::string name = row["name"].as<std::string>();
        int lifespan = row["lifespan"].as<int>(30);

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
        auto result = txn.exec_params(R"(
            SELECT zone_id, id, name, room_description, sector, flags
            FROM "Room"
            WHERE zone_id = $1 AND deleted_at IS NULL
            ORDER BY id
        )", zone_id);

        std::vector<std::unique_ptr<Room>> rooms;
        rooms.reserve(result.size());

        for (const auto& row : result) {
            int local_id = row["id"].as<int>();
            std::string name = row["name"].as<std::string>();
            std::string description = row["room_description"].as<std::string>("");

            // Parse sector type from database enum
            SectorType sector = SectorType::Inside;
            if (!row["sector"].is_null()) {
                std::string sector_str = row["sector"].as<std::string>();
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

                // Parse and set room flags (PostgreSQL array)
                if (!row["flags"].is_null()) {
                    std::string flags_str = row["flags"].as<std::string>();
                    auto flag_names = parse_pg_array(flags_str);
                    // Only log for zone 30 room 1 (temple) to debug the darkness issue
                    if (zone_id == 30 && local_id == 1) {
                        logger->info("DEBUG Temple ({}, {}) raw flags: '{}', parsed {} flags",
                                     zone_id, local_id, flags_str, flag_names.size());
                    }
                    for (const auto& flag_name : flag_names) {
                        auto flag = room_flag_from_db_string(flag_name);
                        if (flag) {
                            room->set_flag(*flag);
                            if (zone_id == 30 && local_id == 1) {
                                logger->info("DEBUG Temple ({}, {}): set flag {}", zone_id, local_id, flag_name);
                            }
                        } else {
                            logger->warn("Room ({}, {}): unknown flag '{}'", zone_id, local_id, flag_name);
                        }
                    }
                }

                rooms.push_back(std::move(room));
            }
        }

        logger->info("Loaded {} rooms for zone {}", rooms.size(), zone_id);
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
        auto result = txn.exec_params(R"(
            SELECT zone_id, id, name, room_description, sector
            FROM "Room"
            WHERE zone_id = $1 AND id = $2
        )", zone_id, room_local_id);

        if (result.empty()) {
            return std::unexpected(Error{ErrorCode::NotFound,
                fmt::format("Room ({}, {}) not found", zone_id, room_local_id)});
        }

        const auto& row = result[0];
        std::string name = row["name"].as<std::string>();
        std::string description = row["room_description"].as<std::string>("");

        auto room = Room::create(
            EntityId(zone_id, room_local_id),
            name,
            SectorType::Inside  // Default sector type
        );

        return room;

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
        auto result = txn.exec_params(R"(
            SELECT direction, to_zone_id, to_room_id, description
            FROM "RoomExit"
            WHERE room_zone_id = $1 AND room_id = $2
        )", zone_id, room_local_id);

        std::vector<ExitData> exits;
        exits.reserve(result.size());

        for (const auto& row : result) {
            ExitData exit;

            // Parse direction enum from database string value
            std::string dir_str = row["direction"].as<std::string>("");
            auto parsed_dir = RoomUtils::parse_direction(dir_str);
            if (!parsed_dir) {
                logger->warn("Invalid direction '{}' for exit in room ({}, {})", dir_str, zone_id, room_local_id);
                continue;  // Skip invalid exits
            }
            exit.direction = parsed_dir.value();

            // Handle nullable destination - skip exits with no destination
            if (row["to_zone_id"].is_null() || row["to_room_id"].is_null()) {
                logger->debug("Exit in room ({}, {}) has null destination, skipping", zone_id, room_local_id);
                continue;
            }
            int to_zone = row["to_zone_id"].as<int>();
            int to_room = row["to_room_id"].as<int>();
            exit.to_room = EntityId(to_zone, to_room);
            exit.description = row["description"].as<std::string>("");

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
        // Query all mobs in this zone
        auto result = txn.exec_params(R"(
            SELECT zone_id, id, name, keywords, room_description, examine_description,
                   level, alignment, armor_class, hit_roll, damage_dice_num,
                   damage_dice_size, damage_dice_bonus, hp_dice_num, hp_dice_size, hp_dice_bonus,
                   strength, intelligence, wisdom, dexterity, constitution, charisma,
                   copper, silver, gold, platinum,
                   race, gender, size, "lifeForce" AS life_force, composition, "damageType" AS damage_type,
                   "mobFlags" AS mob_flags, class_id
            FROM "Mobs"
            WHERE zone_id = $1
            ORDER BY id
        )", zone_id);

        std::vector<std::unique_ptr<Mobile>> mobs;
        mobs.reserve(result.size());

        for (const auto& row : result) {
            int mob_zone_id = row["zone_id"].as<int>();
            int mob_id = row["id"].as<int>();
            EntityId mob_entity_id(mob_zone_id, mob_id);

            std::string mob_name = row["name"].as<std::string>();
            int mob_level = std::max(1, row["level"].as<int>(1));  // Clamp to minimum 1

            // Create the mobile
            auto mob_result = Mobile::create(mob_entity_id, mob_name, mob_level);
            if (!mob_result) {
                logger->error("Failed to create mob ({}, {}): {}",
                             mob_zone_id, mob_id, mob_result.error().message);
                continue;
            }

            auto mob = std::move(*mob_result);

            // Set room description (shown when mob is in room)
            if (!row["room_description"].is_null()) {
                mob->set_ground(row["room_description"].as<std::string>());
            }

            // Set stats from database
            Stats& mob_stats = mob->stats();

            // Base attributes
            if (!row["strength"].is_null()) mob_stats.strength = row["strength"].as<int>();
            if (!row["intelligence"].is_null()) mob_stats.intelligence = row["intelligence"].as<int>();
            if (!row["wisdom"].is_null()) mob_stats.wisdom = row["wisdom"].as<int>();
            if (!row["dexterity"].is_null()) mob_stats.dexterity = row["dexterity"].as<int>();
            if (!row["constitution"].is_null()) mob_stats.constitution = row["constitution"].as<int>();
            if (!row["charisma"].is_null()) mob_stats.charisma = row["charisma"].as<int>();

            // Combat values
            if (!row["armor_class"].is_null()) mob_stats.armor_class = row["armor_class"].as<int>();
            if (!row["hit_roll"].is_null()) mob_stats.hit_roll = row["hit_roll"].as<int>();
            if (!row["alignment"].is_null()) mob_stats.alignment = row["alignment"].as<int>();

            // HP dice - used to calculate HP when spawned
            int hp_num = row["hp_dice_num"].as<int>(1);
            int hp_size = row["hp_dice_size"].as<int>(8);
            int hp_bonus = row["hp_dice_bonus"].as<int>(0);
            mob->set_hp_dice(hp_num, hp_size, hp_bonus);

            // Calculate HP from dice for the prototype
            mob->calculate_hp_from_dice();

            // Set keywords (parse PostgreSQL array format)
            if (!row["keywords"].is_null()) {
                std::string keywords_str = row["keywords"].as<std::string>();
                auto keywords = parse_pg_array(keywords_str);
                if (!keywords.empty()) {
                    mob->set_keywords(std::span<const std::string>(keywords));
                }
            }

            // Set examine description (detailed look text)
            if (!row["examine_description"].is_null()) {
                mob->set_description(row["examine_description"].as<std::string>());
            }

            // Set mob-specific properties
            if (!row["race"].is_null()) {
                mob->set_race(row["race"].as<std::string>());
            }
            if (!row["gender"].is_null()) {
                mob->set_gender(row["gender"].as<std::string>());
            }
            if (!row["size"].is_null()) {
                mob->set_size(row["size"].as<std::string>());
            }
            if (!row["life_force"].is_null()) {
                mob->set_life_force(row["life_force"].as<std::string>());
            }
            if (!row["composition"].is_null()) {
                mob->set_composition(row["composition"].as<std::string>());
            }
            if (!row["damage_type"].is_null()) {
                mob->set_damage_type(row["damage_type"].as<std::string>());
            }

            // Set damage dice for bare hand attacks
            int dam_num = row["damage_dice_num"].as<int>(1);
            int dam_size = row["damage_dice_size"].as<int>(4);
            int dam_bonus = row["damage_dice_bonus"].as<int>(0);
            mob->set_bare_hand_damage(dam_num, dam_size, dam_bonus);

            // Process mob flags (PostgreSQL array of enums)
            if (!row["mob_flags"].is_null()) {
                std::string flags_str = row["mob_flags"].as<std::string>();
                auto flags = parse_pg_array(flags_str);
                for (const auto& flag : flags) {
                    if (flag == "TEACHER") {
                        mob->set_teacher(true);
                    } else if (flag == "AGGRESSIVE") {
                        mob->set_aggressive(true);
                    }
                    // Add more MobFlags as needed
                }
            }

            // Set class_id for class-specific guildmasters
            if (!row["class_id"].is_null()) {
                mob->set_class_id(row["class_id"].as<int>());
            }

            mobs.push_back(std::move(mob));
        }

        logger->info("Loaded {} mobs for zone {}", mobs.size(), zone_id);
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
        auto result = txn.exec_params(R"(
            SELECT zone_id, id, name, keywords, room_description, examine_description,
                   level, alignment, armor_class, hit_roll, damage_dice_num,
                   damage_dice_size, damage_dice_bonus, hp_dice_num, hp_dice_size, hp_dice_bonus,
                   strength, intelligence, wisdom, dexterity, constitution, charisma,
                   copper, silver, gold, platinum,
                   race, gender, size, "lifeForce" AS life_force, composition, "damageType" AS damage_type,
                   "mobFlags" AS mob_flags, class_id
            FROM "Mobs"
            WHERE zone_id = $1 AND id = $2
        )", zone_id, mob_local_id);

        if (result.empty()) {
            return std::unexpected(Error{ErrorCode::NotFound,
                fmt::format("Mob ({}, {}) not found", zone_id, mob_local_id)});
        }

        const auto& row = result[0];
        EntityId mob_entity_id(zone_id, mob_local_id);
        std::string mob_name = row["name"].as<std::string>();
        int mob_level = std::max(1, row["level"].as<int>(1));  // Clamp to minimum 1

        auto mob_result = Mobile::create(mob_entity_id, mob_name, mob_level);
        if (!mob_result) {
            return std::unexpected(mob_result.error());
        }

        auto mob = std::move(*mob_result);

        // Set room description (shown when mob is in room)
        if (!row["room_description"].is_null()) {
            mob->set_ground(row["room_description"].as<std::string>());
        }

        // Set stats
        Stats& mob_stats = mob->stats();
        if (!row["strength"].is_null()) mob_stats.strength = row["strength"].as<int>();
        if (!row["intelligence"].is_null()) mob_stats.intelligence = row["intelligence"].as<int>();
        if (!row["wisdom"].is_null()) mob_stats.wisdom = row["wisdom"].as<int>();
        if (!row["dexterity"].is_null()) mob_stats.dexterity = row["dexterity"].as<int>();
        if (!row["constitution"].is_null()) mob_stats.constitution = row["constitution"].as<int>();
        if (!row["charisma"].is_null()) mob_stats.charisma = row["charisma"].as<int>();
        if (!row["armor_class"].is_null()) mob_stats.armor_class = row["armor_class"].as<int>();
        if (!row["hit_roll"].is_null()) mob_stats.hit_roll = row["hit_roll"].as<int>();
        if (!row["alignment"].is_null()) mob_stats.alignment = row["alignment"].as<int>();

        // HP dice - used to calculate HP when spawned
        int hp_num = row["hp_dice_num"].as<int>(1);
        int hp_size = row["hp_dice_size"].as<int>(8);
        int hp_bonus = row["hp_dice_bonus"].as<int>(0);
        mob->set_hp_dice(hp_num, hp_size, hp_bonus);

        // Calculate HP from dice for the prototype
        mob->calculate_hp_from_dice();

        // Set keywords (parse PostgreSQL array format)
        if (!row["keywords"].is_null()) {
            std::string keywords_str = row["keywords"].as<std::string>();
            auto keywords = parse_pg_array(keywords_str);
            if (!keywords.empty()) {
                mob->set_keywords(std::span<const std::string>(keywords));
            }
        }
        // Set examine description (detailed look text)
        if (!row["examine_description"].is_null()) {
            mob->set_description(row["examine_description"].as<std::string>());
        }

        // Set mob-specific properties
        if (!row["race"].is_null()) {
            mob->set_race(row["race"].as<std::string>());
        }
        if (!row["gender"].is_null()) {
            mob->set_gender(row["gender"].as<std::string>());
        }
        if (!row["size"].is_null()) {
            mob->set_size(row["size"].as<std::string>());
        }
        if (!row["life_force"].is_null()) {
            mob->set_life_force(row["life_force"].as<std::string>());
        }
        if (!row["composition"].is_null()) {
            mob->set_composition(row["composition"].as<std::string>());
        }
        if (!row["damage_type"].is_null()) {
            mob->set_damage_type(row["damage_type"].as<std::string>());
        }

        // Set damage dice for bare hand attacks
        int dam_num = row["damage_dice_num"].as<int>(1);
        int dam_size = row["damage_dice_size"].as<int>(4);
        int dam_bonus = row["damage_dice_bonus"].as<int>(0);
        mob->set_bare_hand_damage(dam_num, dam_size, dam_bonus);

        // Process mob flags (PostgreSQL array of enums)
        if (!row["mob_flags"].is_null()) {
            std::string flags_str = row["mob_flags"].as<std::string>();
            auto flags = parse_pg_array(flags_str);
            for (const auto& flag : flags) {
                if (flag == "TEACHER") {
                    mob->set_teacher(true);
                } else if (flag == "AGGRESSIVE") {
                    mob->set_aggressive(true);
                }
            }
        }

        // Set class_id for class-specific guildmasters
        if (!row["class_id"].is_null()) {
            mob->set_class_id(row["class_id"].as<int>());
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
        // Query all objects in this zone
        auto result = txn.exec_params(R"(
            SELECT zone_id, id, name, keywords, type, room_description, examine_description,
                   weight, cost, level, timer, values, flags
            FROM "Objects"
            WHERE zone_id = $1
            ORDER BY id
        )", zone_id);

        std::vector<std::unique_ptr<Object>> objects;
        objects.reserve(result.size());

        for (const auto& row : result) {
            int obj_zone_id = row["zone_id"].as<int>();
            int obj_id = row["id"].as<int>();
            EntityId obj_entity_id(obj_zone_id, obj_id);

            std::string obj_name = row["name"].as<std::string>();

            // Parse object type from string enum
            std::string type_str = row["type"].as<std::string>();
            ObjectType obj_type = ObjectType::Other;  // Default to Other instead of Undefined

            // Map database ObjectType enum to C++ enum
            if (type_str == "NOTHING") obj_type = ObjectType::Other;  // Map NOTHING to Other
            else if (type_str == "LIGHT") obj_type = ObjectType::Light;
            else if (type_str == "SCROLL") obj_type = ObjectType::Scroll;
            else if (type_str == "WAND") obj_type = ObjectType::Wand;
            else if (type_str == "STAFF") obj_type = ObjectType::Staff;
            else if (type_str == "WEAPON") obj_type = ObjectType::Weapon;
            else if (type_str == "FIREWEAPON") obj_type = ObjectType::Fireweapon;
            else if (type_str == "MISSILE") obj_type = ObjectType::Missile;
            else if (type_str == "TREASURE") obj_type = ObjectType::Treasure;
            else if (type_str == "ARMOR") obj_type = ObjectType::Armor;
            else if (type_str == "POTION") obj_type = ObjectType::Potion;
            else if (type_str == "WORN") obj_type = ObjectType::Worn;
            else if (type_str == "OTHER") obj_type = ObjectType::Other;
            else if (type_str == "TRASH") obj_type = ObjectType::Trash;
            else if (type_str == "TRAP") obj_type = ObjectType::Trap;
            else if (type_str == "CONTAINER") obj_type = ObjectType::Container;
            else if (type_str == "NOTE") obj_type = ObjectType::Note;
            else if (type_str == "DRINKCONTAINER") obj_type = ObjectType::Liquid_Container;
            else if (type_str == "KEY") obj_type = ObjectType::Key;
            else if (type_str == "FOOD") obj_type = ObjectType::Food;
            else if (type_str == "MONEY") obj_type = ObjectType::Money;
            else if (type_str == "PEN") obj_type = ObjectType::Pen;
            else if (type_str == "BOAT") obj_type = ObjectType::Boat;
            else if (type_str == "FOUNTAIN") obj_type = ObjectType::Fountain;
            else if (type_str == "PORTAL") obj_type = ObjectType::Portal;
            else if (type_str == "SPELLBOOK") obj_type = ObjectType::Spellbook;
            else if (type_str == "BOARD") obj_type = ObjectType::Board;
            // Map database-only types to closest C++ equivalent
            else if (type_str == "ROPE") obj_type = ObjectType::Other;
            else if (type_str == "WALL") obj_type = ObjectType::Other;
            else if (type_str == "TOUCHSTONE") obj_type = ObjectType::Other;
            else if (type_str == "INSTRUMENT") obj_type = ObjectType::Other;

            // Create the object
            auto obj_result = Object::create(obj_entity_id, obj_name, obj_type);
            if (!obj_result) {
                logger->error("Failed to create object ({}, {}): {}",
                             obj_zone_id, obj_id, obj_result.error().message);
                continue;
            }

            auto obj = std::move(*obj_result);

            // Set weight
            if (!row["weight"].is_null()) {
                obj->set_weight(static_cast<int>(row["weight"].as<double>()));
            }

            // Set cost/value
            if (!row["cost"].is_null()) {
                obj->set_value(row["cost"].as<int>());
            }

            // Set level
            if (!row["level"].is_null()) {
                obj->set_level(row["level"].as<int>());
            }

            // Set timer
            if (!row["timer"].is_null()) {
                obj->set_timer(row["timer"].as<int>());
            }

            objects.push_back(std::move(obj));
        }

        logger->info("Loaded {} objects for zone {}", objects.size(), zone_id);
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
        auto result = txn.exec_params(R"(
            SELECT zone_id, id, name, keywords, type, room_description, examine_description,
                   weight, cost, level, timer, values, flags
            FROM "Objects"
            WHERE zone_id = $1 AND id = $2
        )", zone_id, object_local_id);

        if (result.empty()) {
            return std::unexpected(Error{ErrorCode::NotFound,
                fmt::format("Object ({}, {}) not found", zone_id, object_local_id)});
        }

        const auto& row = result[0];
        EntityId obj_entity_id(zone_id, object_local_id);
        std::string obj_name = row["name"].as<std::string>();

        // Parse object type from database
        ObjectType obj_type = ObjectType::Other;  // Default to Other
        if (!row["type"].is_null()) {
            std::string type_str = row["type"].as<std::string>();
            // Map database ObjectType enum to C++ enum
            if (type_str == "NOTHING") obj_type = ObjectType::Other;
            else if (type_str == "LIGHT") obj_type = ObjectType::Light;
            else if (type_str == "SCROLL") obj_type = ObjectType::Scroll;
            else if (type_str == "WAND") obj_type = ObjectType::Wand;
            else if (type_str == "STAFF") obj_type = ObjectType::Staff;
            else if (type_str == "WEAPON") obj_type = ObjectType::Weapon;
            else if (type_str == "FIREWEAPON") obj_type = ObjectType::Fireweapon;
            else if (type_str == "MISSILE") obj_type = ObjectType::Missile;
            else if (type_str == "TREASURE") obj_type = ObjectType::Treasure;
            else if (type_str == "ARMOR") obj_type = ObjectType::Armor;
            else if (type_str == "POTION") obj_type = ObjectType::Potion;
            else if (type_str == "WORN") obj_type = ObjectType::Worn;
            else if (type_str == "OTHER") obj_type = ObjectType::Other;
            else if (type_str == "TRASH") obj_type = ObjectType::Trash;
            else if (type_str == "TRAP") obj_type = ObjectType::Trap;
            else if (type_str == "CONTAINER") obj_type = ObjectType::Container;
            else if (type_str == "NOTE") obj_type = ObjectType::Note;
            else if (type_str == "DRINKCONTAINER") obj_type = ObjectType::Liquid_Container;
            else if (type_str == "KEY") obj_type = ObjectType::Key;
            else if (type_str == "FOOD") obj_type = ObjectType::Food;
            else if (type_str == "MONEY") obj_type = ObjectType::Money;
            else if (type_str == "PEN") obj_type = ObjectType::Pen;
            else if (type_str == "BOAT") obj_type = ObjectType::Boat;
            else if (type_str == "FOUNTAIN") obj_type = ObjectType::Fountain;
            else if (type_str == "PORTAL") obj_type = ObjectType::Portal;
            else if (type_str == "SPELLBOOK") obj_type = ObjectType::Spellbook;
            else if (type_str == "BOARD") obj_type = ObjectType::Board;
            else if (type_str == "ROPE") obj_type = ObjectType::Other;
            else if (type_str == "WALL") obj_type = ObjectType::Other;
            else if (type_str == "TOUCHSTONE") obj_type = ObjectType::Other;
            else if (type_str == "INSTRUMENT") obj_type = ObjectType::Other;
        }

        auto obj_result = Object::create(obj_entity_id, obj_name, obj_type);
        if (!obj_result) {
            return std::unexpected(obj_result.error());
        }

        auto obj = std::move(*obj_result);

        // Set descriptions
        if (!row["room_description"].is_null()) {
            obj->set_description(row["room_description"].as<std::string>());
        }
        // examine_description stored in short_desc for detailed look
        if (!row["examine_description"].is_null()) {
            obj->set_short_description(row["examine_description"].as<std::string>());
        }

        // Set basic properties
        if (!row["weight"].is_null()) {
            obj->set_weight(row["weight"].as<int>());
        }
        if (!row["cost"].is_null()) {
            obj->set_value(row["cost"].as<int>());
        }
        if (!row["level"].is_null()) {
            obj->set_level(row["level"].as<int>());
        }
        if (!row["timer"].is_null()) {
            obj->set_timer(row["timer"].as<int>());
        }

        // Set keywords (parse PostgreSQL array format)
        if (!row["keywords"].is_null()) {
            std::string keywords_str = row["keywords"].as<std::string>();
            auto keywords = parse_pg_array(keywords_str);
            if (!keywords.empty()) {
                obj->set_keywords(std::span<const std::string>(keywords));
            }
        }

        // Parse object values JSON - can be JSON object or array
        // Note: Modern Object class uses a single value, not legacy values[0-3]
        if (!row["values"].is_null()) {
            try {
                std::string values_str = row["values"].as<std::string>();
                auto values_json = nlohmann::json::parse(values_str);
                if (values_json.is_array() && !values_json.empty() && values_json[0].is_number()) {
                    // First value often represents primary object-specific data
                    // For now, we store it in object value if cost wasn't set
                    if (row["cost"].is_null()) {
                        obj->set_value(values_json[0].get<int>());
                    }
                } else if (values_json.is_object()) {
                    // New format: JSON object with named keys
                    // Could be used to store weapon damage, armor values, etc.
                }
            } catch (const nlohmann::json::exception& e) {
                logger->warn("Failed to parse values for object ({}, {}): {}",
                            zone_id, object_local_id, e.what());
            }
        }

        // Parse object flags (PostgreSQL array format)
        if (!row["flags"].is_null()) {
            std::string flags_str = row["flags"].as<std::string>();
            auto flag_names = parse_pg_array(flags_str);
            for (const auto& flag_name : flag_names) {
                auto flag = magic_enum::enum_cast<ObjectFlag>(flag_name);
                if (flag) {
                    obj->set_flag(flag.value());
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

            // Parse direction
            std::string dir_str = row["direction"].as<std::string>();
            auto dir = direction_from_db_string(dir_str);
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

        logger->info("Loaded {} exits for zone {}", exits.size(), zone_id);
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

        logger->info("Loaded {} mob resets for zone {}", resets.size(), zone_id);
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

        logger->info("Loaded {} mob equipment items for zone {}", equipment.size(), zone_id);
        return equipment;

    } catch (const pqxx::sql_error& e) {
        logger->error("SQL error loading mob equipment for zone {}: {}", zone_id, e.what());
        return std::unexpected(Error{ErrorCode::InternalError,
            fmt::format("Failed to load mob equipment: {}", e.what())});
    }
}

Result<std::vector<ObjectResetData>> load_object_resets_in_zone(pqxx::work& txn, int zone_id) {
    auto logger = Log::database();
    logger->debug("Loading object resets for zone {} from database", zone_id);

    try {
        auto result = txn.exec_params(R"(
            SELECT
                id, object_zone_id, object_id,
                room_zone_id, room_id, container_reset_id,
                max_instances, probability, comment
            FROM "ObjectResets"
            WHERE zone_id = $1
            ORDER BY id
        )", zone_id);

        std::vector<ObjectResetData> resets;
        resets.reserve(result.size());

        for (const auto& row : result) {
            ObjectResetData reset;
            reset.id = row["id"].as<int>();
            reset.object_id = EntityId(
                row["object_zone_id"].as<int>(),
                row["object_id"].as<int>()
            );

            // Room is optional (null for objects inside containers)
            if (!row["room_zone_id"].is_null() && !row["room_id"].is_null()) {
                reset.room_id = EntityId(
                    row["room_zone_id"].as<int>(),
                    row["room_id"].as<int>()
                );
            } else {
                reset.room_id = INVALID_ENTITY_ID;
            }

            // Container reset ID (0 if not inside a container)
            reset.container_reset_id = row["container_reset_id"].as<int>(0);

            reset.max_instances = row["max_instances"].as<int>(1);
            reset.probability = row["probability"].as<float>(1.0f);
            if (!row["comment"].is_null()) {
                reset.comment = row["comment"].as<std::string>();
            }
            resets.push_back(std::move(reset));
        }

        logger->info("Loaded {} object resets for zone {}", resets.size(), zone_id);
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
                id, name, plain_name, description,
                "abilityType" AS ability_type, "minPosition" AS min_position, violent,
                cast_time_rounds, cooldown_ms,
                is_area, sphere, damage_type,
                pages, memorization_time, quest_only, humanoid_only
            FROM "Ability"
            ORDER BY id
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
            ability.sphere = row["sphere"].is_null() ? "" : row["sphere"].as<std::string>();
            ability.damage_type = row["damage_type"].is_null() ? "" : row["damage_type"].as<std::string>();
            ability.pages = row["pages"].is_null() ? 0 : row["pages"].as<int>();
            ability.memorization_time = row["memorization_time"].as<int>(0);
            ability.quest_only = row["quest_only"].as<bool>(false);
            ability.humanoid_only = row["humanoid_only"].as<bool>(false);
            abilities.push_back(std::move(ability));
        }

        logger->info("Loaded {} abilities from database", abilities.size());
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
                id, name, plain_name, description,
                "abilityType" AS ability_type, "minPosition" AS min_position, violent,
                cast_time_rounds, cooldown_ms,
                is_area, sphere, damage_type,
                pages, memorization_time, quest_only, humanoid_only
            FROM "Ability"
            WHERE id = $1
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
        ability.sphere = row["sphere"].is_null() ? "" : row["sphere"].as<std::string>();
        ability.damage_type = row["damage_type"].is_null() ? "" : row["damage_type"].as<std::string>();
        ability.pages = row["pages"].is_null() ? 0 : row["pages"].as<int>();
        ability.memorization_time = row["memorization_time"].as<int>(0);
        ability.quest_only = row["quest_only"].as<bool>(false);
        ability.humanoid_only = row["humanoid_only"].as<bool>(false);

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
        auto result = txn.exec_params(R"(
            SELECT ability_id, class_id, min_level
            FROM "ClassSkills"
            WHERE class_id = $1
            ORDER BY min_level, ability_id
        )", class_id);

        std::vector<ClassAbilityData> abilities;
        abilities.reserve(result.size());

        for (const auto& row : result) {
            ClassAbilityData ability;
            ability.ability_id = row["ability_id"].as<int>();
            ability.class_id = row["class_id"].as<int>();
            ability.min_level = row["min_level"].as<int>();
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
        // Single efficient query joining CharacterAbilities with Ability
        auto result = txn.exec_params(R"(
            SELECT
                ca.ability_id,
                a.name,
                a.plain_name,
                ca.known,
                ca.proficiency,
                a."abilityType" AS ability_type,
                a.violent
            FROM "CharacterAbilities" ca
            JOIN "Ability" a ON a.id = ca.ability_id
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
            ability.known = row["known"].as<bool>(false);
            ability.proficiency = row["proficiency"].as<int>(0);
            ability.violent = row["violent"].as<bool>(false);

            // Convert ability type string to enum
            std::string type_str = row["ability_type"].as<std::string>("SPELL");
            if (type_str == "SKILL") {
                ability.type = AbilityType::Skill;
            } else if (type_str == "CHANT") {
                ability.type = AbilityType::Chant;
            } else if (type_str == "SONG") {
                ability.type = AbilityType::Song;
            } else {
                ability.type = AbilityType::Spell;
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
        // Build IN clause
        std::string id_list;
        for (size_t i = 0; i < ability_ids.size(); ++i) {
            if (i > 0) id_list += ",";
            id_list += std::to_string(ability_ids[i]);
        }

        auto result = txn.exec(fmt::format(R"(
            SELECT
                ability_id, effect_id,
                override_params::text AS override_params,
                "order", trigger, chance_pct, condition
            FROM "AbilityEffect"
            WHERE ability_id IN ({})
            ORDER BY ability_id, "order"
        )", id_list));

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

    // Helper to convert vector<string> to PostgreSQL array literal
    std::string to_pg_array(const std::vector<std::string>& vec) {
        if (vec.empty()) {
            return "{}";
        }
        std::string result = "{";
        for (size_t i = 0; i < vec.size(); ++i) {
            if (i > 0) result += ",";
            result += "\"" + vec[i] + "\"";
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

        // Format as UUID string
        char uuid[37];
        snprintf(uuid, sizeof(uuid),
            "%08x-%04x-%04x-%04x-%012llx",
            static_cast<unsigned int>(a >> 32),
            static_cast<unsigned int>((a >> 16) & 0xFFFF),
            static_cast<unsigned int>(a & 0xFFFF),
            static_cast<unsigned int>(b >> 48),
            static_cast<unsigned long long>(b & 0xFFFFFFFFFFFFULL));
        return std::string(uuid);
    }

    // Bcrypt-style password hashing using crypt() with SHA-512
    // Format: $6$<salt>$<hash> (SHA-512 crypt)
    std::string hash_password(const std::string& password) {
        // Generate random salt for SHA-512 crypt
        unsigned char salt_bytes[16];
        RAND_bytes(salt_bytes, sizeof(salt_bytes));

        // Encode salt as base64-like string
        static const char* b64 = "./0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
        std::string salt = "$6$";  // SHA-512 prefix
        for (int i = 0; i < 16; ++i) {
            salt += b64[salt_bytes[i] % 64];
        }
        salt += "$";

        // Use crypt_r for thread safety
        struct crypt_data data;
        memset(&data, 0, sizeof(data));
        char* result = crypt_r(password.c_str(), salt.c_str(), &data);

        if (result == nullptr) {
            // Fallback to simple hash if crypt fails
            auto logger = Log::database();
            logger->error("crypt_r failed, using fallback hash");
            std::size_t hash = std::hash<std::string>{}(password);
            return fmt::format("fallback${:016x}", hash);
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
                struct crypt_data data;
                memset(&data, 0, sizeof(data));
                char* result = crypt_r(password.c_str(), stored_hash.c_str(), &data);
                return result && stored_hash == result;
            }

            case HashType::BcryptHash: {
                // Bcrypt hash ($2a$, $2b$, $2y$ format) - use crypt_r if supported
                // libxcrypt on modern Linux supports bcrypt via crypt_r
                struct crypt_data data;
                memset(&data, 0, sizeof(data));
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
                struct crypt_data data;
                memset(&data, 0, sizeof(data));
                char* result = crypt_r(password.c_str(), stored_hash.c_str(), &data);
                if (!result) return false;
                // Legacy hashes compare first 10 characters
                return std::strncmp(result, stored_hash.c_str(), 10) == 0;
            }

            case HashType::FallbackHash: {
                // Simple development hash
                std::size_t hash = std::hash<std::string>{}(password);
                return stored_hash == fmt::format("fallback${:016x}", hash);
            }

            case HashType::Unknown:
            default:
                // Try direct comparison as last resort
                return stored_hash == password;
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
                id, name, level, alignment,
                strength, intelligence, wisdom, dexterity, constitution, charisma, luck,
                hit_points, hit_points_max, movement, movement_max,
                copper, silver, gold, platinum,
                bank_copper, bank_silver, bank_gold, bank_platinum,
                password_hash, race_type, gender, player_class, class_id,
                current_room, save_room, home_room,
                hit_roll, damage_roll, armor_class,
                last_login, time_played, is_online,
                hunger, thirst,
                description, title,
                prompt, page_length,
                player_flags, effect_flags, privilege_flags,
                experience, skill_points,
                created_at, updated_at
            FROM "Characters"
            WHERE UPPER(name) = UPPER($1)
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
        character.movement = row["movement"].as<int>(100);
        character.movement_max = row["movement_max"].as<int>(100);

        // Currency
        character.copper = row["copper"].as<int>(0);
        character.silver = row["silver"].as<int>(0);
        character.gold = row["gold"].as<int>(0);
        character.platinum = row["platinum"].as<int>(0);
        character.bank_copper = row["bank_copper"].as<int>(0);
        character.bank_silver = row["bank_silver"].as<int>(0);
        character.bank_gold = row["bank_gold"].as<int>(0);
        character.bank_platinum = row["bank_platinum"].as<int>(0);

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

        // Location
        if (!row["current_room"].is_null()) {
            character.current_room = row["current_room"].as<int>();
        }
        if (!row["save_room"].is_null()) {
            character.save_room = row["save_room"].as<int>();
        }
        if (!row["home_room"].is_null()) {
            character.home_room = row["home_room"].as<int>();
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

        // Flags (PostgreSQL arrays)
        if (!row["player_flags"].is_null()) {
            character.player_flags = parse_pg_string_array(row["player_flags"].as<std::string>());
        }
        if (!row["effect_flags"].is_null()) {
            character.effect_flags = parse_pg_string_array(row["effect_flags"].as<std::string>());
        }
        if (!row["privilege_flags"].is_null()) {
            character.privilege_flags = parse_pg_string_array(row["privilege_flags"].as<std::string>());
        }

        // Experience
        character.experience = row["experience"].as<int>(0);
        character.skill_points = row["skill_points"].as<int>(0);

        logger->info("Loaded character '{}' (id: {})", character.name, character.id);
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
                id, name, level, alignment,
                strength, intelligence, wisdom, dexterity, constitution, charisma, luck,
                hit_points, hit_points_max, movement, movement_max,
                copper, silver, gold, platinum,
                bank_copper, bank_silver, bank_gold, bank_platinum,
                password_hash, race_type, gender, player_class, class_id,
                current_room, save_room, home_room,
                hit_roll, damage_roll, armor_class,
                last_login, time_played, is_online,
                hunger, thirst,
                description, title,
                prompt, page_length,
                player_flags, effect_flags, privilege_flags,
                experience, skill_points,
                created_at, updated_at
            FROM "Characters"
            WHERE id = $1
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
        character.movement = row["movement"].as<int>(100);
        character.movement_max = row["movement_max"].as<int>(100);
        character.copper = row["copper"].as<int>(0);
        character.silver = row["silver"].as<int>(0);
        character.gold = row["gold"].as<int>(0);
        character.platinum = row["platinum"].as<int>(0);
        character.bank_copper = row["bank_copper"].as<int>(0);
        character.bank_silver = row["bank_silver"].as<int>(0);
        character.bank_gold = row["bank_gold"].as<int>(0);
        character.bank_platinum = row["bank_platinum"].as<int>(0);
        character.password_hash = row["password_hash"].as<std::string>("");
        character.race_type = row["race_type"].as<std::string>("human");
        character.gender = row["gender"].as<std::string>("neutral");
        if (!row["player_class"].is_null()) {
            character.player_class = row["player_class"].as<std::string>();
        }
        if (!row["class_id"].is_null()) {
            character.class_id = row["class_id"].as<int>();
        }
        if (!row["current_room"].is_null()) {
            character.current_room = row["current_room"].as<int>();
        }
        if (!row["save_room"].is_null()) {
            character.save_room = row["save_room"].as<int>();
        }
        if (!row["home_room"].is_null()) {
            character.home_room = row["home_room"].as<int>();
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
        if (!row["player_flags"].is_null()) {
            character.player_flags = parse_pg_string_array(row["player_flags"].as<std::string>());
        }
        if (!row["effect_flags"].is_null()) {
            character.effect_flags = parse_pg_string_array(row["effect_flags"].as<std::string>());
        }
        if (!row["privilege_flags"].is_null()) {
            character.privilege_flags = parse_pg_string_array(row["privilege_flags"].as<std::string>());
        }
        character.experience = row["experience"].as<int>(0);
        character.skill_points = row["skill_points"].as<int>(0);

        logger->info("Loaded character '{}' by ID", character.name);
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
            logger->info("Upgraded password hash for character '{}' from legacy to SHA-512", name);
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
                hit_points, hit_points_max, movement, movement_max,
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
        character.movement = 100;
        character.movement_max = 100;

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
        txn.exec_params(R"(
            UPDATE "Characters" SET
                level = $2, alignment = $3,
                strength = $4, intelligence = $5, wisdom = $6, dexterity = $7,
                constitution = $8, charisma = $9, luck = $10,
                hit_points = $11, hit_points_max = $12, movement = $13, movement_max = $14,
                copper = $15, silver = $16, gold = $17, platinum = $18,
                bank_copper = $19, bank_silver = $20, bank_gold = $21, bank_platinum = $22,
                current_room = $23, save_room = $24,
                hit_roll = $25, damage_roll = $26, armor_class = $27,
                time_played = $28, hunger = $29, thirst = $30,
                experience = $31, skill_points = $32,
                updated_at = NOW()
            WHERE id = $1
        )",
            character.id,
            character.level, character.alignment,
            character.strength, character.intelligence, character.wisdom, character.dexterity,
            character.constitution, character.charisma, character.luck,
            character.hit_points, character.hit_points_max, character.movement, character.movement_max,
            character.copper, character.silver, character.gold, character.platinum,
            character.bank_copper, character.bank_silver, character.bank_gold, character.bank_platinum,
            character.current_room.value_or(0), character.save_room.value_or(0),
            character.hit_roll, character.damage_roll, character.armor_class,
            character.time_played, character.hunger, character.thirst,
            character.experience, character.skill_points
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
    int room_id) {
    auto logger = Log::database();
    logger->debug("Saving location for character {}: room {}", character_id, room_id);

    try {
        txn.exec_params(R"(
            UPDATE "Characters" SET current_room = $2, save_room = $2, updated_at = NOW() WHERE id = $1
        )", character_id, room_id);

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
        logger->info("Loaded user '{}' (id: {})", user.username, user.id);
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
        logger->info("Loaded user '{}' by email", user.username);
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
        logger->info("Loaded user '{}' by ID", user.username);
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
                logger->info("Upgraded password hash for user '{}' from legacy to SHA-512", username_or_email);
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

        logger->info("Loaded {} characters for user {}", characters.size(), user_id);
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
        // Convert time_point to timestamp
        auto time_t_until = std::chrono::system_clock::to_time_t(until);
        std::tm tm_until;
        gmtime_r(&time_t_until, &tm_until);
        char buffer[32];
        strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &tm_until);

        txn.exec_params(R"(
            UPDATE "Users"
            SET locked_until = $2::timestamp,
                updated_at = NOW()
            WHERE id = $1
        )", user_id, std::string(buffer));

        logger->info("Locked user account {} until {}", user_id, buffer);
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

        logger->info("Linked character {} to user {}", character_id, user_id);
        return {};

    } catch (const pqxx::sql_error& e) {
        logger->error("SQL error linking character to user: {}", e.what());
        return std::unexpected(Error{ErrorCode::InternalError,
            fmt::format("Failed to link character to user: {}", e.what())});
    }
}

} // namespace WorldQueries
