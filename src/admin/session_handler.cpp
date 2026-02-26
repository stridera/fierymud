#include "session_handler.hpp"

#include <fmt/format.h>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

#include "core/config.hpp"
#include "core/player.hpp"
#include "database/connection_pool.hpp"
#include "database/player_queries.hpp"
#include "database/world_queries.hpp"
#include "server/mud_server.hpp"
#include "server/world_server.hpp"
#include "world/world_manager.hpp"

using json = nlohmann::json;

namespace fierymud {

/// Track virtual sessions (players created via API without a network connection).
/// These players have no PlayerOutput, so messages accumulate in output_queue_.
static std::unordered_map<std::string, std::shared_ptr<Player>> virtual_sessions_;

std::shared_ptr<Player> find_virtual_session(std::string_view name) {
    auto it = virtual_sessions_.find(std::string(name));
    if (it != virtual_sessions_.end()) {
        return it->second;
    }
    return nullptr;
}

void register_session_handlers(AdminServer &admin_server, ModernMUDServer &mud_server) {
    // POST /api/admin/session/create - Create a virtual test player session
    admin_server.register_handler(
        "/api/admin/session/create",
        [&mud_server]([[maybe_unused]] const std::string &path, const std::string &body) -> std::string {
            spdlog::info("Received session create request");

            try {
                auto request = json::parse(body);

                if (!request.contains("player_name")) {
                    json error_json = {{"error", "Bad Request"}, {"message", "Missing required field: player_name"}};
                    return error_json.dump();
                }

                std::string player_name = request["player_name"].get<std::string>();

                // Check if session already exists
                if (virtual_sessions_.count(player_name)) {
                    auto &existing = virtual_sessions_[player_name];
                    auto room = existing->current_room();
                    json response = {{"success", true},
                                     {"message", fmt::format("Session for '{}' already exists", player_name)},
                                     {"player",
                                      {{"name", std::string(existing->name())},
                                       {"level", existing->level()},
                                       {"god_level", existing->god_level()},
                                       {"room_zone_id", room ? room->id().zone_id() : 0},
                                       {"room_id", room ? room->id().local_id() : 0}}}};
                    return response.dump();
                }

                // Check if already logged in via telnet
                if (mud_server.find_player(player_name)) {
                    json error_json = {
                        {"error", "Conflict"},
                        {"message", fmt::format("Player '{}' is already logged in via telnet", player_name)}};
                    return error_json.dump();
                }

                // Load player from database
                auto load_result = ConnectionPool::instance().execute(
                    [&player_name](pqxx::work &txn) -> Result<std::unique_ptr<Player>> {
                        return PlayerQueries::load_player_by_name(txn, player_name);
                    });

                if (!load_result) {
                    json error_json = {{"error", "Not Found"},
                                       {"message", fmt::format("Player '{}' not found in database: {}", player_name,
                                                               load_result.error().message)}};
                    return error_json.dump();
                }

                auto player = std::shared_ptr<Player>(std::move(*load_result));

                // Set as god for full access (Overlord level)
                player->set_god_level(100);
                player->set_online(true);

                // Load all abilities at max proficiency (god privilege)
                auto all_abilities_result = ConnectionPool::instance().execute(
                    [](pqxx::work &txn) -> Result<std::vector<WorldQueries::AbilityData>> {
                        return WorldQueries::load_all_abilities(txn);
                    });

                if (all_abilities_result) {
                    for (const auto &ability_data : *all_abilities_result) {
                        LearnedAbility learned;
                        learned.ability_id = ability_data.id;
                        learned.name = ability_data.name;
                        learned.plain_name = ability_data.plain_name;
                        learned.description = ability_data.description;
                        learned.known = true;
                        learned.proficiency = 1000;
                        learned.violent = ability_data.violent;
                        learned.min_level = 1;

                        switch (ability_data.type) {
                        case WorldQueries::AbilityType::Spell:
                            learned.type = "SPELL";
                            break;
                        case WorldQueries::AbilityType::Skill:
                            learned.type = "SKILL";
                            break;
                        case WorldQueries::AbilityType::Chant:
                            learned.type = "CHANT";
                            break;
                        case WorldQueries::AbilityType::Song:
                            learned.type = "SONG";
                            break;
                        }

                        player->set_ability(learned);
                    }
                    spdlog::info("Granted {} abilities to virtual session '{}'", all_abilities_result->size(),
                                 player_name);
                }

                // Do NOT set output_ — leave it as nullptr so messages only
                // accumulate in output_queue_ without being sent anywhere

                // Place in starting room
                EntityId target_room_id = player->start_room();
                if (!target_room_id.is_valid()) {
                    target_room_id = Config::instance().default_starting_room();
                }

                auto &world = WorldManager::instance();
                auto move_result = world.move_actor_to_room(player, target_room_id);
                if (!move_result.success) {
                    // Try default room as fallback
                    target_room_id = Config::instance().default_starting_room();
                    move_result = world.move_actor_to_room(player, target_room_id);
                    if (!move_result.success) {
                        json error_json = {{"error", "Internal Server Error"},
                                           {"message", fmt::format("Could not place '{}' in any room: {}", player_name,
                                                                   move_result.failure_reason)}};
                        return error_json.dump();
                    }
                }

                // Register with the world server so execute_command can find them
                auto *ws = WorldServer::instance();
                if (ws) {
                    ws->add_player(player);
                }

                // Clear any output generated during setup
                player->clear_output_queue();

                // Store in virtual sessions map
                auto room = player->current_room();
                virtual_sessions_[player_name] = player;

                spdlog::info("Virtual session created for '{}' in room {}:{}", player_name,
                             room ? room->id().zone_id() : 0, room ? room->id().local_id() : 0);

                json response = {{"success", true},
                                 {"message", fmt::format("Virtual session created for '{}'", player_name)},
                                 {"player",
                                  {{"name", std::string(player->name())},
                                   {"level", player->level()},
                                   {"god_level", player->god_level()},
                                   {"class", player->player_class()},
                                   {"race", std::string(player->race())},
                                   {"room_zone_id", room ? room->id().zone_id() : 0},
                                   {"room_id", room ? room->id().local_id() : 0}}}};

                return response.dump();

            } catch (const json::parse_error &e) {
                spdlog::error("Session create JSON parse error: {}", e.what());
                json error_json = {{"error", "Bad Request"}, {"message", "Invalid JSON in request body"}};
                return error_json.dump();
            } catch (const std::exception &e) {
                spdlog::error("Session create error: {}", e.what());
                json error_json = {{"error", "Internal Server Error"}, {"message", e.what()}};
                return error_json.dump();
            }
        });

    // POST /api/admin/session/destroy - Remove a virtual player from the world
    admin_server.register_handler(
        "/api/admin/session/destroy",
        [&mud_server]([[maybe_unused]] const std::string &path, const std::string &body) -> std::string {
            (void)mud_server;
            spdlog::info("Received session destroy request");

            try {
                auto request = json::parse(body);

                if (!request.contains("player_name")) {
                    json error_json = {{"error", "Bad Request"}, {"message", "Missing required field: player_name"}};
                    return error_json.dump();
                }

                std::string player_name = request["player_name"].get<std::string>();

                auto it = virtual_sessions_.find(player_name);
                if (it == virtual_sessions_.end()) {
                    json error_json = {{"error", "Not Found"},
                                       {"message", fmt::format("No virtual session for '{}'", player_name)}};
                    return error_json.dump();
                }

                auto player = it->second;

                // Remove from room
                if (auto room = player->current_room()) {
                    room->remove_actor(player->id());
                }

                player->set_online(false);

                // Remove from virtual sessions
                virtual_sessions_.erase(it);

                spdlog::info("Virtual session destroyed for '{}'", player_name);

                json response = {{"success", true},
                                 {"message", fmt::format("Virtual session for '{}' destroyed", player_name)}};
                return response.dump();

            } catch (const json::parse_error &e) {
                spdlog::error("Session destroy JSON parse error: {}", e.what());
                json error_json = {{"error", "Bad Request"}, {"message", "Invalid JSON in request body"}};
                return error_json.dump();
            } catch (const std::exception &e) {
                spdlog::error("Session destroy error: {}", e.what());
                json error_json = {{"error", "Internal Server Error"}, {"message", e.what()}};
                return error_json.dump();
            }
        });

    // GET /api/admin/session/output - Get accumulated output for a virtual player
    admin_server.register_handler(
        "/api/admin/session/output",
        []([[maybe_unused]] const std::string &path, const std::string &body) -> std::string {
            spdlog::debug("Received session output request");

            try {
                // Parse query params from path or body
                // Support both GET with query params and POST with body
                std::string player_name;
                bool clear = true;

                if (!body.empty()) {
                    auto request = json::parse(body);
                    player_name = request.value("player_name", "");
                    clear = request.value("clear", true);
                }

                if (player_name.empty()) {
                    json error_json = {{"error", "Bad Request"}, {"message", "Missing player_name"}};
                    return error_json.dump();
                }

                auto it = virtual_sessions_.find(player_name);
                if (it == virtual_sessions_.end()) {
                    json error_json = {{"error", "Not Found"},
                                       {"message", fmt::format("No virtual session for '{}'", player_name)}};
                    return error_json.dump();
                }

                auto &player = it->second;
                auto output = player->get_output_queue();

                if (clear) {
                    player->clear_output_queue();
                }

                json response = {{"success", true}, {"player_name", player_name}, {"output", output}};
                return response.dump();

            } catch (const json::parse_error &e) {
                spdlog::error("Session output JSON parse error: {}", e.what());
                json error_json = {{"error", "Bad Request"}, {"message", "Invalid JSON in request body"}};
                return error_json.dump();
            } catch (const std::exception &e) {
                spdlog::error("Session output error: {}", e.what());
                json error_json = {{"error", "Internal Server Error"}, {"message", e.what()}};
                return error_json.dump();
            }
        });

    spdlog::info("Registered session handlers with admin server");
}

} // namespace fierymud
