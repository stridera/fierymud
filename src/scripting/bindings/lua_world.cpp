#include "lua_world.hpp"

#include "../../core/actor.hpp"
#include "../../core/object.hpp"
#include "../../world/world_manager.hpp"

#define SOL_ALL_SAFETIES_ON 1
#include <algorithm>
#include <cctype>
#include <sol/sol.hpp>
#include <spdlog/spdlog.h>

namespace FieryMUD {

void register_world_bindings(sol::state &lua) {
    auto world_table = lua.create_named_table("world");

    // world.find_player(name) - Find a player by name (case-insensitive)
    // Returns: Player or nil
    world_table["find_player"] = [](const std::string &name) -> std::shared_ptr<Player> {
        if (name.empty()) {
            return nullptr;
        }

        auto &world = WorldManager::instance();
        auto player = world.find_player(name);

        if (player) {
            spdlog::debug("world.find_player: found '{}'", name);
        }

        return player;
    };

    // world.find_mobile(name) - Find a mobile by name in the world
    // Returns: Mobile or nil
    world_table["find_mobile"] = [](const std::string &name) -> std::shared_ptr<Mobile> {
        if (name.empty()) {
            return nullptr;
        }

        auto &world = WorldManager::instance();
        auto mobile = world.find_mobile(name);

        if (mobile) {
            spdlog::debug("world.find_mobile: found '{}'", name);
        }

        return mobile;
    };

    // world.count_mobiles(keyword) - Count mobiles in the world by prototype ID (as string)
    // Used for checking if a specific mob type exists anywhere in the world
    // Returns: int (count of matching mobiles)
    world_table["count_mobiles"] = [](const std::string &keyword) -> int {
        auto &world = WorldManager::instance();
        int count = 0;

        // Parse keyword as prototype ID (e.g., "48915" for zone 489, local 15)
        // or as a vnum string
        try {
            auto vnum = static_cast<std::uint64_t>(std::stoll(keyword));
            int zone_id = static_cast<int>(vnum / 100);
            int local_id = static_cast<int>(vnum % 100);
            EntityId prototype_id(zone_id, local_id);

            world.for_each_mobile([&count, &prototype_id](const std::shared_ptr<Mobile> &mob) {
                if (mob && mob->prototype_id() == prototype_id) {
                    count++;
                }
            });
        } catch (...) {
            // If not a number, try matching by name keyword
            world.for_each_mobile([&count, &keyword](const std::shared_ptr<Mobile> &mob) {
                if (mob) {
                    std::string name = std::string(mob->name());
                    std::transform(name.begin(), name.end(), name.begin(), ::tolower);
                    std::string search = keyword;
                    std::transform(search.begin(), search.end(), search.begin(), ::tolower);
                    if (name.find(search) != std::string::npos) {
                        count++;
                    }
                }
            });
        }

        return count;
    };

    // world.count_objects(keyword) - Count objects in the world by prototype ID (as string)
    // Note: This counts objects in rooms, not in inventories
    // Returns: int (count of matching objects)
    world_table["count_objects"] = [](const std::string &keyword) -> int {
        auto &world = WorldManager::instance();
        int count = 0;

        // Parse keyword as prototype ID (e.g., "48926" for zone 489, local 26)
        try {
            auto vnum = static_cast<std::uint64_t>(std::stoll(keyword));
            int zone_id = static_cast<int>(vnum / 100);
            int local_id = static_cast<int>(vnum % 100);
            EntityId prototype_id(zone_id, local_id);

            // Iterate through all rooms and count matching objects
            // This is expensive but matches DG Script semantics
            for (auto &zone : world.get_all_zones()) {
                for (auto &room : world.get_rooms_in_zone(zone->id())) {
                    if (room) {
                        for (auto &obj : room->contents().objects) {
                            if (obj && obj->id() == prototype_id) {
                                count++;
                            }
                        }
                    }
                }
            }
        } catch (...) {
            // If not a number, try matching by name keyword
            for (auto &zone : world.get_all_zones()) {
                for (auto &room : world.get_rooms_in_zone(zone->id())) {
                    if (room) {
                        for (auto &obj : room->contents().objects) {
                            if (obj) {
                                std::string name = std::string(obj->name());
                                std::transform(name.begin(), name.end(), name.begin(), ::tolower);
                                std::string search = keyword;
                                std::transform(search.begin(), search.end(), search.begin(), ::tolower);
                                if (name.find(search) != std::string::npos) {
                                    count++;
                                }
                            }
                        }
                    }
                }
            }
        }

        return count;
    };

    // world.destroy(entity) - Remove an entity (mobile or object) from the world
    // Returns: (bool success, string? error)
    world_table["destroy"] = [](sol::object entity) -> std::tuple<bool, sol::optional<std::string>> {
        auto &world = WorldManager::instance();

        // Try as Mobile first
        if (entity.is<std::shared_ptr<Mobile>>()) {
            auto mobile = entity.as<std::shared_ptr<Mobile>>();
            if (!mobile) {
                return std::make_tuple(false, std::string("invalid_target"));
            }

            auto result = world.destroy_mobile(mobile);
            if (!result) {
                return std::make_tuple(false, result.error().message);
            }

            spdlog::debug("world.destroy: destroyed mobile");
            return std::make_tuple(true, sol::optional<std::string>{});
        }

        // Try as Object
        if (entity.is<std::shared_ptr<Object>>()) {
            auto object = entity.as<std::shared_ptr<Object>>();
            if (!object) {
                return std::make_tuple(false, std::string("invalid_target"));
            }

            auto result = world.destroy_object(object);
            if (!result) {
                return std::make_tuple(false, result.error().message);
            }

            spdlog::debug("world.destroy: destroyed object");
            return std::make_tuple(true, sol::optional<std::string>{});
        }

        return std::make_tuple(false, std::string("invalid_target"));
    };

    spdlog::debug("Registered world Lua bindings");
}

} // namespace FieryMUD
