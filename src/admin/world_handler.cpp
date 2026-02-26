#include "world_handler.hpp"

#include <fmt/format.h>
#include <magic_enum/magic_enum.hpp>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

#include "core/actor.hpp"
#include "core/combat.hpp"
#include "core/mobile.hpp"
#include "core/player.hpp"
#include "server/mud_server.hpp"
#include "server/world_server.hpp"
#include "session_handler.hpp"
#include "world/room.hpp"
#include "world/world_manager.hpp"

using json = nlohmann::json;

namespace fierymud {

namespace {

/// Parse "zoneId/id" from the end of a path like "/api/admin/room/30/1"
bool parse_entity_id_from_path(const std::string &path, const std::string &prefix, int &zone_id, int &local_id) {
    if (!path.starts_with(prefix)) {
        return false;
    }
    auto rest = path.substr(prefix.size());
    auto slash = rest.find('/');
    if (slash == std::string::npos) {
        return false;
    }
    try {
        zone_id = std::stoi(rest.substr(0, slash));
        local_id = std::stoi(rest.substr(slash + 1));
        return true;
    } catch (...) {
        return false;
    }
}

/// Parse a single name from the end of a path like "/api/admin/actor/TestBot"
std::string parse_name_from_path(const std::string &path, const std::string &prefix) {
    if (!path.starts_with(prefix)) {
        return "";
    }
    return path.substr(prefix.size());
}

/// Serialize room exits to JSON
json exits_to_json(const Room &room) {
    json exits = json::object();
    auto available = room.get_available_exits();
    for (auto dir : available) {
        auto *exit_info = room.get_exit(dir);
        if (!exit_info) {
            continue;
        }
        std::string dir_name;
        switch (dir) {
        case Direction::North:
            dir_name = "north";
            break;
        case Direction::East:
            dir_name = "east";
            break;
        case Direction::South:
            dir_name = "south";
            break;
        case Direction::West:
            dir_name = "west";
            break;
        case Direction::Up:
            dir_name = "up";
            break;
        case Direction::Down:
            dir_name = "down";
            break;
        default:
            dir_name = "unknown";
            break;
        }
        exits[dir_name] = {{"to_room_zone", exit_info->to_room.zone_id()},
                           {"to_room_id", exit_info->to_room.local_id()},
                           {"has_door", exit_info->has_door},
                           {"is_closed", exit_info->is_closed},
                           {"is_locked", exit_info->is_locked}};
    }
    return exits;
}

/// Serialize actor stats to JSON
json stats_to_json(const Stats &stats) {
    return {{"level", stats.level},
            {"hit_points", stats.hit_points},
            {"max_hit_points", stats.max_hit_points},
            {"stamina", stats.stamina},
            {"max_stamina", stats.max_stamina},
            {"strength", stats.strength},
            {"dexterity", stats.dexterity},
            {"intelligence", stats.intelligence},
            {"wisdom", stats.wisdom},
            {"constitution", stats.constitution},
            {"charisma", stats.charisma},
            {"accuracy", stats.accuracy},
            {"evasion", stats.evasion},
            {"attack_power", stats.attack_power},
            {"armor_rating", stats.armor_rating},
            {"alignment", stats.alignment},
            {"experience", stats.experience}};
}

} // namespace

void register_world_handlers(AdminServer &admin_server, ModernMUDServer &mud_server) {
    // POST /api/admin/world/pause
    admin_server.register_handler(
        "/api/admin/world/pause",
        []([[maybe_unused]] const std::string &path, [[maybe_unused]] const std::string &body) -> std::string {
            auto *ws = WorldServer::instance();
            if (!ws) {
                return json({{"error", "World server not available"}}).dump();
            }
            ws->pause();
            return json({{"success", true}, {"message", "World paused"}, {"paused", true}}).dump();
        });

    // POST /api/admin/world/unpause
    admin_server.register_handler(
        "/api/admin/world/unpause",
        []([[maybe_unused]] const std::string &path, [[maybe_unused]] const std::string &body) -> std::string {
            auto *ws = WorldServer::instance();
            if (!ws) {
                return json({{"error", "World server not available"}}).dump();
            }
            ws->unpause();
            return json({{"success", true}, {"message", "World unpaused"}, {"paused", false}}).dump();
        });

    // GET /api/admin/world/status
    admin_server.register_handler(
        "/api/admin/world/status",
        []([[maybe_unused]] const std::string &path, [[maybe_unused]] const std::string &body) -> std::string {
            auto *ws = WorldServer::instance();
            if (!ws) {
                return json({{"error", "World server not available"}}).dump();
            }
            return json({{"success", true},
                         {"running", ws->is_running()},
                         {"paused", ws->is_paused()},
                         {"active_players", ws->active_player_count()}})
                .dump();
        });

    // POST /api/admin/world/tick
    admin_server.register_handler(
        "/api/admin/world/tick", []([[maybe_unused]] const std::string &path, const std::string &body) -> std::string {
            auto *ws = WorldServer::instance();
            if (!ws) {
                return json({{"error", "World server not available"}}).dump();
            }
            if (!ws->is_paused()) {
                return json({{"error", "Bad Request"}, {"message", "World must be paused to use tick"}}).dump();
            }

            int count = 1;
            if (!body.empty()) {
                try {
                    auto request = json::parse(body);
                    count = request.value("count", 1);
                } catch (...) {
                }
            }
            count = std::clamp(count, 1, 100);

            ws->tick(count);

            return json({{"success", true}, {"message", fmt::format("Advanced {} tick(s)", count)}, {"ticks", count}})
                .dump();
        });

    // GET /api/admin/room/* - Room details (path: /api/admin/room/{zoneId}/{id})
    admin_server.register_handler(
        "/api/admin/room",
        []([[maybe_unused]] const std::string &path, [[maybe_unused]] const std::string &body) -> std::string {
            int zone_id = 0;
            int local_id = 0;
            if (!parse_entity_id_from_path(path, "/api/admin/room/", zone_id, local_id)) {
                return json({{"error", "Bad Request"}, {"message", "Path must be /api/admin/room/{zoneId}/{id}"}})
                    .dump();
            }

            auto room = WorldManager::instance().get_room(
                EntityId{static_cast<uint32_t>(zone_id), static_cast<uint32_t>(local_id)});
            if (!room) {
                return json({{"error", "Not Found"},
                             {"message", fmt::format("Room {}:{} not found", zone_id, local_id)}})
                    .dump();
            }

            // Build actor list
            json actors = json::array();
            for (const auto &actor : room->contents().actors) {
                if (actor) {
                    actors.push_back({{"name", std::string(actor->name())},
                                      {"type", std::string(actor->type_name())},
                                      {"level", actor->stats().level},
                                      {"hp", actor->stats().hit_points},
                                      {"max_hp", actor->stats().max_hit_points},
                                      {"is_fighting", actor->is_fighting()}});
                }
            }

            // Build object list
            json objects = json::array();
            for (const auto &obj : room->contents().objects) {
                if (obj) {
                    objects.push_back({{"name", std::string(obj->name())},
                                       {"id_zone", obj->id().zone_id()},
                                       {"id_local", obj->id().local_id()}});
                }
            }

            json response = {{"success", true},
                             {"room",
                              {{"zone_id", zone_id},
                               {"id", local_id},
                               {"name", std::string(room->name())},
                               {"description", std::string(room->description())},
                               {"exits", exits_to_json(*room)},
                               {"actors", actors},
                               {"objects", objects}}}};

            return response.dump();
        });

    // GET /api/admin/mob/* - Mob prototype details
    admin_server.register_handler(
        "/api/admin/mob",
        []([[maybe_unused]] const std::string &path, [[maybe_unused]] const std::string &body) -> std::string {
            int zone_id = 0;
            int local_id = 0;
            if (!parse_entity_id_from_path(path, "/api/admin/mob/", zone_id, local_id)) {
                return json({{"error", "Bad Request"}, {"message", "Path must be /api/admin/mob/{zoneId}/{id}"}})
                    .dump();
            }

            EntityId mob_id{static_cast<uint32_t>(zone_id), static_cast<uint32_t>(local_id)};
            auto *prototype = WorldManager::instance().get_mobile_prototype(mob_id);
            if (!prototype) {
                return json(
                           {{"error", "Not Found"}, {"message", fmt::format("Mob {}:{} not found", zone_id, local_id)}})
                    .dump();
            }

            auto aggro = prototype->aggro_condition();
            json response = {{"success", true},
                             {"mob",
                              {{"zone_id", zone_id},
                               {"id", local_id},
                               {"name", std::string(prototype->name())},
                               {"description", std::string(prototype->description())},
                               {"stats", stats_to_json(prototype->stats())},
                               {"is_aggressive", prototype->is_aggressive()},
                               {"aggro_condition", aggro ? *aggro : ""},
                               {"race", std::string(prototype->race())},
                               {"gender", std::string(prototype->gender())}}}};

            return response.dump();
        });

    // GET /api/admin/actor/* - Live actor state (path: /api/admin/actor/{name})
    admin_server.register_handler(
        "/api/admin/actor",
        [&mud_server]([[maybe_unused]] const std::string &path,
                      [[maybe_unused]] const std::string &body) -> std::string {
            std::string actor_name = parse_name_from_path(path, "/api/admin/actor/");
            if (actor_name.empty()) {
                return json({{"error", "Bad Request"}, {"message", "Path must be /api/admin/actor/{name}"}}).dump();
            }

            // Search online players
            auto player = mud_server.find_player(actor_name);
            std::shared_ptr<Actor> actor;
            if (player) {
                actor = std::static_pointer_cast<Actor>(player);
            }

            // Search spawned mobiles if not found as player
            if (!actor) {
                actor = std::static_pointer_cast<Actor>(WorldManager::instance().find_mobile(actor_name));
            }

            if (!actor) {
                return json({{"error", "Not Found"}, {"message", fmt::format("Actor '{}' not found", actor_name)}})
                    .dump();
            }

            auto room = actor->current_room();

            json effects = json::array();
            for (const auto &effect : actor->active_effects()) {
                json eff = {{"name", effect.name},
                            {"source", effect.source},
                            {"duration_hours", effect.duration_hours},
                            {"permanent", effect.is_permanent()}};
                effects.push_back(eff);
            }

            json actor_json = {{"name", std::string(actor->name())},
                               {"type", std::string(actor->type_name())},
                               {"stats", stats_to_json(actor->stats())},
                               {"position", std::string(magic_enum::enum_name(actor->position()))},
                               {"is_fighting", actor->is_fighting()},
                               {"is_alive", actor->is_alive()},
                               {"race", std::string(actor->race())},
                               {"gender", std::string(actor->gender())},
                               {"effects", effects},
                               {"room_zone_id", room ? static_cast<int>(room->id().zone_id()) : 0},
                               {"room_id", room ? static_cast<int>(room->id().local_id()) : 0}};

            // Add player-specific fields
            if (player) {
                actor_json["wallet"] = player->wallet().value();
                actor_json["bank"] = player->bank().value();
            }

            return json({{"success", true}, {"actor", actor_json}}).dump();
        });

    // POST /api/admin/spawn - Spawn mob or object into the world
    admin_server.register_handler(
        "/api/admin/spawn", []([[maybe_unused]] const std::string &path, const std::string &body) -> std::string {
            spdlog::info("Received spawn request");

            try {
                auto request = json::parse(body);
                std::string type = request.value("type", "mob");
                int zone_id = request.value("zoneId", 0);
                int id = request.value("id", 0);
                int room_zone = request.value("room_zone", zone_id);
                int room_id = request.value("room_id", 0);

                EntityId entity_id{static_cast<uint32_t>(zone_id), static_cast<uint32_t>(id)};
                EntityId room_entity_id{static_cast<uint32_t>(room_zone), static_cast<uint32_t>(room_id)};

                if (type == "mob") {
                    auto spawned = WorldManager::instance().spawn_mobile_to_room(entity_id, room_entity_id);
                    if (!spawned) {
                        return json({{"error", "Failed"},
                                     {"message", fmt::format("Could not spawn mob {}:{} in room {}:{}", zone_id, id,
                                                             room_zone, room_id)}})
                            .dump();
                    }
                    return json({{"success", true},
                                 {"message", fmt::format("Spawned '{}' ({}:{}) in room {}:{}", spawned->name(), zone_id,
                                                         id, room_zone, room_id)},
                                 {"mob",
                                  {{"name", std::string(spawned->name())},
                                   {"hp", spawned->stats().hit_points},
                                   {"max_hp", spawned->stats().max_hit_points}}}})
                        .dump();
                } else if (type == "object") {
                    auto obj = WorldManager::instance().create_object_instance(entity_id);
                    if (!obj) {
                        return json({{"error", "Failed"},
                                     {"message", fmt::format("Could not create object {}:{}", zone_id, id)}})
                            .dump();
                    }
                    // Place in room
                    auto room = WorldManager::instance().get_room(room_entity_id);
                    if (!room) {
                        return json({{"error", "Not Found"},
                                     {"message", fmt::format("Room {}:{} not found", room_zone, room_id)}})
                            .dump();
                    }
                    room->add_object(obj);
                    return json({{"success", true},
                                 {"message",
                                  fmt::format("Spawned object '{}' in room {}:{}", obj->name(), room_zone, room_id)}})
                        .dump();
                }

                return json({{"error", "Bad Request"}, {"message", "type must be 'mob' or 'object'"}}).dump();

            } catch (const json::parse_error &e) {
                return json({{"error", "Bad Request"}, {"message", "Invalid JSON"}}).dump();
            } catch (const std::exception &e) {
                spdlog::error("Spawn handler error: {}", e.what());
                return json({{"error", "Internal Server Error"}, {"message", e.what()}}).dump();
            }
        });

    // POST /api/admin/teleport - Move a player to a specific room directly
    admin_server.register_handler(
        "/api/admin/teleport",
        [&mud_server]([[maybe_unused]] const std::string &path, const std::string &body) -> std::string {
            spdlog::info("Received teleport request");

            try {
                auto request = json::parse(body);

                if (!request.contains("player_name") || !request.contains("zone_id") || !request.contains("room_id")) {
                    return json({{"error", "Bad Request"},
                                 {"message", "Missing required fields: player_name, zone_id, room_id"}})
                        .dump();
                }

                std::string player_name = request["player_name"].get<std::string>();
                int zone_id = request["zone_id"].get<int>();
                int room_id = request["room_id"].get<int>();

                // Find the player (telnet or virtual session)
                auto player = mud_server.find_player(player_name);
                if (!player) {
                    player = find_virtual_session(player_name);
                }
                if (!player) {
                    return json({{"error", "Not Found"},
                                 {"message", fmt::format("Player '{}' not found online", player_name)}})
                        .dump();
                }

                EntityId target_room{static_cast<uint32_t>(zone_id), static_cast<uint32_t>(room_id)};
                auto from_room = player->current_room();
                auto result = WorldManager::instance().move_actor_to_room(player, target_room);

                if (!result.success) {
                    return json({{"error", "Failed"},
                                 {"message", fmt::format("Teleport failed: {}", result.failure_reason)}})
                        .dump();
                }

                return json({{"success", true},
                             {"from_room",
                              from_room ? fmt::format("{}:{}", from_room->id().zone_id(), from_room->id().local_id())
                                        : "none"},
                             {"to_room", fmt::format("{}:{}", zone_id, room_id)}})
                    .dump();

            } catch (const json::parse_error &e) {
                return json({{"error", "Bad Request"}, {"message", "Invalid JSON"}}).dump();
            } catch (const std::exception &e) {
                spdlog::error("Teleport handler error: {}", e.what());
                return json({{"error", "Internal Server Error"}, {"message", e.what()}}).dump();
            }
        });

    spdlog::info("Registered world handlers with admin server");
}

} // namespace fierymud
