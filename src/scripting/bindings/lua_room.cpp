#include "lua_room.hpp"
#include "../../world/room.hpp"
#include "../../world/world_manager.hpp"
#include "../../core/actor.hpp"
#include "../../core/object.hpp"

#define SOL_ALL_SAFETIES_ON 1
#include <sol/sol.hpp>

#include <fmt/format.h>
#include <spdlog/spdlog.h>

namespace FieryMUD {

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
        }

        // Note: spawn_mob and spawn_obj will be added in Phase 3
        // when WorldManager integration is complete
    );

    // Direction helper functions
    lua.set_function("reverse_direction", [](Direction dir) -> Direction {
        return RoomUtils::get_opposite_direction(dir);
    });

    lua.set_function("direction_name", [](Direction dir) -> std::string {
        return std::string(RoomUtils::get_direction_name(dir));
    });

    // Global get_room function for script compatibility
    // Accepts: number (legacy vnum), string ("zone:id"), or Room object
    lua.set_function("get_room", [](sol::object arg) -> std::shared_ptr<Room> {
        if (arg.is<std::shared_ptr<Room>>()) {
            // If already a Room, just return it
            return arg.as<std::shared_ptr<Room>>();
        }

        if (arg.is<int>() || arg.is<double>()) {
            // Legacy vnum format - convert to EntityId
            auto vnum = static_cast<std::uint64_t>(arg.as<double>());
            return WorldManager::instance().get_room(EntityId{vnum});
        }

        if (arg.is<std::string>()) {
            // String format "zone:local_id"
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
            // Try parsing as single number
            try {
                auto vnum = static_cast<std::uint64_t>(std::stoll(id_str));
                return WorldManager::instance().get_room(EntityId{vnum});
            } catch (...) {
                spdlog::warn("get_room: Cannot parse room ID: {}", id_str);
                return nullptr;
            }
        }

        spdlog::warn("get_room: Invalid argument type");
        return nullptr;
    });

    spdlog::debug("Lua Room bindings registered");
}

} // namespace FieryMUD
