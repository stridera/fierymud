#include "lua_room.hpp"
#include "../../world/room.hpp"
#include "../../world/world_manager.hpp"
#include "../../core/actor.hpp"
#include "../../core/object.hpp"

#define SOL_ALL_SAFETIES_ON 1
#include <sol/sol.hpp>

#include <fmt/format.h>
#include <spdlog/spdlog.h>
#include <algorithm>
#include <unordered_map>

namespace FieryMUD {

namespace {

// Parse direction string to Direction enum
std::optional<Direction> parse_direction(const std::string& dir_str) {
    static const std::unordered_map<std::string, Direction> dir_map = {
        {"north", Direction::North}, {"n", Direction::North},
        {"east", Direction::East}, {"e", Direction::East},
        {"south", Direction::South}, {"s", Direction::South},
        {"west", Direction::West}, {"w", Direction::West},
        {"up", Direction::Up}, {"u", Direction::Up},
        {"down", Direction::Down}, {"d", Direction::Down},
        {"northeast", Direction::Northeast}, {"ne", Direction::Northeast},
        {"northwest", Direction::Northwest}, {"nw", Direction::Northwest},
        {"southeast", Direction::Southeast}, {"se", Direction::Southeast},
        {"southwest", Direction::Southwest}, {"sw", Direction::Southwest},
        {"in", Direction::In},
        {"out", Direction::Out}
    };

    std::string lower = dir_str;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

    auto it = dir_map.find(lower);
    if (it != dir_map.end()) {
        return it->second;
    }
    return std::nullopt;
}

} // anonymous namespace

void register_room_bindings(sol::state& lua) {
    // Direction enum
    lua.new_enum<Direction>("Direction",
        {
            {"North", Direction::North},
            {"East", Direction::East},
            {"South", Direction::South},
            {"West", Direction::West},
            {"Up", Direction::Up},
            {"Down", Direction::Down},
            {"Northeast", Direction::Northeast},
            {"Northwest", Direction::Northwest},
            {"Southeast", Direction::Southeast},
            {"Southwest", Direction::Southwest},
            {"In", Direction::In},
            {"Out", Direction::Out},
            {"Portal", Direction::Portal}
        }
    );

    // SectorType enum
    lua.new_enum<SectorType>("SectorType",
        {
            {"Inside", SectorType::Inside},
            {"City", SectorType::City},
            {"Field", SectorType::Field},
            {"Forest", SectorType::Forest},
            {"Hills", SectorType::Hills},
            {"Mountains", SectorType::Mountains},
            {"WaterSwim", SectorType::Water_Swim},
            {"WaterNoswim", SectorType::Water_Noswim},
            {"Underwater", SectorType::Underwater},
            {"Flying", SectorType::Flying},
            {"Desert", SectorType::Desert},
            {"Swamp", SectorType::Swamp},
            {"Beach", SectorType::Beach},
            {"Road", SectorType::Road},
            {"Underground", SectorType::Underground},
            {"Lava", SectorType::Lava},
            {"Ice", SectorType::Ice},
            {"Astral", SectorType::Astral},
            {"Fire", SectorType::Fire},
            {"Lightning", SectorType::Lightning},
            {"Spirit", SectorType::Spirit},
            {"Badlands", SectorType::Badlands},
            {"Void", SectorType::Void}
        }
    );

    // RoomFlag enum REMOVED - replaced by:
    // - base_light_level for lighting
    // - Lua room restriction scripts for access control
    // - Sector type for environment
    // - Capacity for occupancy limits

    // Room class
    lua.new_usertype<Room>("Room",
        sol::no_constructor,

        // Basic properties
        "name", sol::property([](const Room& r) { return std::string(r.name()); }),
        "id", sol::property([](const Room& r) -> std::string {
            auto id = r.id();
            return fmt::format("{}:{}", id.zone_id(), id.local_id());
        }),
        "zone_id", sol::property([](const Room& r) -> std::uint32_t {
            return r.zone_id().zone_id();
        }),

        // Sector and environment
        "sector", sol::property(&Room::sector_type),
        "sector_name", sol::property([](const Room& r) -> std::string {
            return std::string(RoomUtils::get_sector_name(r.sector_type()));
        }),
        "base_light_level", sol::property(&Room::base_light_level),
        "effective_light", sol::property(&Room::calculate_effective_light),
        "capacity", sol::property(&Room::capacity),

        // State checks
        "is_dark", sol::property(&Room::is_dark),
        "is_peaceful", sol::property(&Room::is_peaceful),
        "allows_magic", sol::property(&Room::allows_magic),
        "allows_recall", sol::property(&Room::allows_recall),
        "allows_summon", sol::property(&Room::allows_summon),
        "allows_teleport", sol::property(&Room::allows_teleport),
        "is_death_trap", sol::property(&Room::is_death_trap),
        "is_full", sol::property(&Room::is_full),

        // Entry restriction (Lua script for access control)
        "entry_restriction", sol::property([](const Room& r) -> std::string {
            return r.entry_restriction();
        }),
        "has_entry_restriction", sol::property(&Room::has_entry_restriction),

        // Contents - actors
        "actors", sol::property([](Room& r) -> sol::as_table_t<std::vector<std::shared_ptr<Actor>>> {
            return sol::as_table(r.contents().actors);
        }),

        "actor_count", sol::property([](const Room& r) -> int {
            return static_cast<int>(r.contents().actors.size());
        }),

        // Contents - objects
        "objects", sol::property([](Room& r) -> sol::as_table_t<std::vector<std::shared_ptr<Object>>> {
            std::vector<std::shared_ptr<Object>> result;
            for (auto& obj : r.contents().objects) {
                result.push_back(obj);
            }
            return sol::as_table(result);
        }),

        "object_count", sol::property([](const Room& r) -> int {
            return static_cast<int>(r.contents().objects.size());
        }),

        // Exit information
        "exits", sol::property([](const Room& r) -> sol::as_table_t<std::vector<Direction>> {
            return sol::as_table(r.get_available_exits());
        }),

        "has_exit", [](const Room& r, Direction dir) -> bool {
            return r.has_exit(dir);
        },

        "get_exit_room", [](const Room& r, Direction dir) -> std::shared_ptr<Room> {
            const auto* exit = r.get_exit(dir);
            if (!exit || !exit->to_room.is_valid()) {
                return nullptr;
            }
            return WorldManager::instance().get_room(exit->to_room);
        },

        "exit_is_open", [](const Room& r, Direction dir) -> bool {
            const auto* exit = r.get_exit(dir);
            if (!exit) return false;
            return exit->is_passable();
        },

        "exit_is_locked", [](const Room& r, Direction dir) -> bool {
            const auto* exit = r.get_exit(dir);
            if (!exit) return false;
            return exit->is_locked;
        },

        // Communication methods
        "send", [](Room& r, const std::string& msg) {
            for (auto& actor : r.contents().actors) {
                actor->send_message(msg);
            }
        },

        "send_except", [](Room& r, std::shared_ptr<Actor> except, const std::string& msg) {
            for (auto& actor : r.contents().actors) {
                if (actor != except) {
                    actor->send_message(msg);
                }
            }
        },

        // Finding entities
        "find_actor", [](Room& r, const std::string& keyword) -> std::shared_ptr<Actor> {
            auto actors = r.find_actors_by_keyword(keyword);
            return actors.empty() ? nullptr : actors[0];
        },

        "find_actors", [](Room& r, const std::string& keyword)
            -> sol::as_table_t<std::vector<std::shared_ptr<Actor>>> {
            return sol::as_table(r.find_actors_by_keyword(keyword));
        },

        "find_object", [](Room& r, const std::string& keyword) -> std::shared_ptr<Object> {
            auto objects = r.find_objects_by_keyword(keyword);
            return objects.empty() ? nullptr : objects[0];
        },

        "find_objects", [](Room& r, const std::string& keyword)
            -> sol::as_table_t<std::vector<std::shared_ptr<Object>>> {
            return sol::as_table(r.find_objects_by_keyword(keyword));
        },

        // Get adjacent room by direction string (enables chaining: room:dir('n'):at(fn))
        // Returns the room in that direction, or nil if no exit
        "dir", [](const Room& r, const std::string& dir_str) -> std::shared_ptr<Room> {
            auto dir = parse_direction(dir_str);
            if (!dir) {
                spdlog::warn("room:dir: Invalid direction '{}'", dir_str);
                return nullptr;
            }
            const auto* exit = r.get_exit(*dir);
            if (!exit || !exit->to_room.is_valid()) {
                return nullptr;
            }
            return WorldManager::instance().get_room(exit->to_room);
        },

        // Execute a function in this room's context
        // Usage: room:at(function() ... end)
        // The function receives the room as its first argument
        "at", [](std::shared_ptr<Room> room, sol::protected_function fn) -> sol::protected_function_result {
            if (!room || !fn.valid()) {
                return sol::protected_function_result();
            }
            // Call the function with the room as context
            return fn(room);
        },

        // Spawn a mobile into this room
        // Usage: room:spawn_mobile(zone_id, local_id)
        // Returns the mobile if successful, nil otherwise
        "spawn_mobile", [](std::shared_ptr<Room> room, int zone_id, int local_id) -> std::shared_ptr<Mobile> {
            if (!room) return nullptr;

            EntityId prototype_id(zone_id, local_id);
            auto mobile = WorldManager::instance().spawn_mobile_to_room(prototype_id, room->id());
            if (!mobile) {
                spdlog::warn("room:spawn_mobile: Failed to spawn mobile {}:{}", zone_id, local_id);
                return nullptr;
            }

            spdlog::debug("room:spawn_mobile: Spawned mobile {}:{} in room {}", zone_id, local_id, room->id().to_string());
            return mobile;
        },

        // Spawn an object into this room
        // Usage: room:spawn_object(zone_id, local_id)
        // Returns the object if successful, nil otherwise
        "spawn_object", [](std::shared_ptr<Room> room, int zone_id, int local_id) -> std::shared_ptr<Object> {
            if (!room) return nullptr;

            EntityId prototype_id(zone_id, local_id);
            auto object = WorldManager::instance().create_object_instance(prototype_id);
            if (!object) {
                spdlog::warn("room:spawn_object: Failed to create object {}:{}", zone_id, local_id);
                return nullptr;
            }

            // Add object to room contents
            room->add_object(object);

            spdlog::debug("room:spawn_object: Spawned object {}:{} in room {}", zone_id, local_id, room->id().to_string());
            return object;
        },

        // Remove all NPCs and objects from this room (purge)
        // Returns number of entities removed
        "purge", [](std::shared_ptr<Room> room) -> int {
            if (!room) return 0;

            auto& world = WorldManager::instance();
            int removed = 0;

            // Get copies of the lists since we'll be modifying them
            auto actors_copy = room->contents().actors;
            auto objects_copy = room->contents().objects;

            // Remove mobiles (not players)
            for (auto& actor : actors_copy) {
                if (dynamic_cast<Mobile*>(actor.get())) {
                    auto result = world.destroy_mobile(std::dynamic_pointer_cast<Mobile>(actor));
                    if (result) {
                        removed++;
                    }
                }
            }

            // Remove objects
            for (auto& obj : objects_copy) {
                auto result = world.destroy_object(obj);
                if (result) {
                    removed++;
                }
            }

            spdlog::debug("room:purge: Removed {} entities from room {}", removed, room->id().to_string());
            return removed;
        }
    );

    // Direction helper functions
    lua.set_function("reverse_direction", [](Direction dir) -> Direction {
        return RoomUtils::get_opposite_direction(dir);
    });

    lua.set_function("direction_name", [](Direction dir) -> std::string {
        return std::string(RoomUtils::get_direction_name(dir));
    });

    // Global get_room function - accepts (zone_id, local_id) composite format
    // This is the modern API - two separate integer parameters
    lua.set_function("get_room", sol::overload(
        // Modern format: get_room(zone_id, local_id)
        [](int zone_id, int local_id) -> std::shared_ptr<Room> {
            return WorldManager::instance().get_room(EntityId(zone_id, local_id));
        },
        // Single arg overload for compatibility
        [](sol::object arg) -> std::shared_ptr<Room> {
            if (arg.is<std::shared_ptr<Room>>()) {
                return arg.as<std::shared_ptr<Room>>();
            }

            if (arg.is<int>() || arg.is<double>()) {
                // Legacy numeric format - convert to EntityId
                auto legacy_id = static_cast<std::uint64_t>(arg.as<double>());
                return WorldManager::instance().get_room(EntityId{legacy_id});
            }

            if (arg.is<std::string>()) {
                std::string id_str = arg.as<std::string>();
                auto colon_pos = id_str.find(':');
                if (colon_pos != std::string::npos) {
                    try {
                        int zone_id = std::stoi(id_str.substr(0, colon_pos));
                        int local_id = std::stoi(id_str.substr(colon_pos + 1));
                        return WorldManager::instance().get_room(EntityId(zone_id, local_id));
                    } catch (...) {
                        spdlog::warn("get_room: Invalid room ID format: {}", id_str);
                        return nullptr;
                    }
                }
                try {
                    auto legacy_id = static_cast<std::uint64_t>(std::stoll(id_str));
                    return WorldManager::instance().get_room(EntityId{legacy_id});
                } catch (...) {
                    spdlog::warn("get_room: Cannot parse room ID: {}", id_str);
                    return nullptr;
                }
            }

            spdlog::warn("get_room: Invalid argument type");
            return nullptr;
        }
    ));

    spdlog::debug("Lua Room bindings registered");
}

} // namespace FieryMUD
