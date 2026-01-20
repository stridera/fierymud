#include "player_handler.hpp"
#include "../server/mud_server.hpp"
#include "../core/actor.hpp"
#include "../world/room.hpp"
#include <fmt/format.h>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

using json = nlohmann::json;

namespace fierymud {

void register_player_handlers(AdminServer& admin_server, ModernMUDServer& mud_server) {
    // GET /api/admin/players - List online players
    admin_server.register_handler("/api/admin/players", [&mud_server]([[maybe_unused]] const std::string& path, [[maybe_unused]] const std::string& body) -> std::string {
        spdlog::info("Received players list request");

        try {
            auto players = mud_server.get_online_players();
            json players_json = json::array();

            for (const auto& player : players) {
                if (player) {
                    auto room = player->current_room_ptr();
                    json player_obj = {
                        {"name", std::string(player->name())},
                        {"level", player->level()},
                        {"class", player->player_class()},
                        {"race", std::string(player->race())},
                        {"room_id", room ? room->id().local_id() : 0},
                        {"god_level", player->god_level()},
                        {"is_linkdead", player->is_linkdead()}
                    };
                    players_json.push_back(player_obj);
                }
            }

            json response = {
                {"success", true},
                {"player_count", players_json.size()},
                {"players", players_json}
            };

            return response.dump();

        } catch (const std::exception& e) {
            spdlog::error("Players list handler error: {}", e.what());
            json error_json = {
                {"error", "Internal Server Error"},
                {"message", e.what()}
            };
            return error_json.dump();
        }
    });

    // POST /api/admin/command - Execute a god command
    admin_server.register_handler("/api/admin/command", [&mud_server]([[maybe_unused]] const std::string& path, const std::string& body) -> std::string {
        spdlog::info("Received command execution request");

        try {
            auto request = json::parse(body);

            if (!request.contains("command")) {
                json error_json = {
                    {"error", "Bad Request"},
                    {"message", "Missing required field: command"}
                };
                return error_json.dump();
            }

            std::string command = request["command"].get<std::string>();
            std::string executor = request.value("executor", "AdminAPI");

            spdlog::info("Executing command from {}: {}", executor, command);

            // Execute command through the server
            // TODO: Implement command execution through CommandSystem
            // For now, we'll return a placeholder response
            json response = {
                {"success", true},
                {"message", fmt::format("Command '{}' queued for execution", command)},
                {"executor", executor},
                {"note", "Full command execution integration pending"}
            };

            return response.dump();

        } catch (const json::parse_error& e) {
            spdlog::error("Command handler JSON parse error: {}", e.what());
            json error_json = {
                {"error", "Bad Request"},
                {"message", "Invalid JSON in request body"}
            };
            return error_json.dump();
        } catch (const std::exception& e) {
            spdlog::error("Command handler error: {}", e.what());
            json error_json = {
                {"error", "Internal Server Error"},
                {"message", e.what()}
            };
            return error_json.dump();
        }
    });

    // POST /api/admin/broadcast - Send message to all players
    admin_server.register_handler("/api/admin/broadcast", [&mud_server]([[maybe_unused]] const std::string& path, const std::string& body) -> std::string {
        spdlog::info("Received broadcast request");

        try {
            auto request = json::parse(body);

            if (!request.contains("message")) {
                json error_json = {
                    {"error", "Bad Request"},
                    {"message", "Missing required field: message"}
                };
                return error_json.dump();
            }

            std::string message = request["message"].get<std::string>();
            std::string sender = request.value("sender", "System");

            // Format the broadcast message
            std::string formatted = fmt::format("\r\n[{}] {}\r\n", sender, message);

            mud_server.broadcast_message(formatted);

            spdlog::info("Broadcast sent from {}: {}", sender, message);

            json response = {
                {"success", true},
                {"message", "Broadcast sent successfully"},
                {"recipient_count", mud_server.online_player_count()}
            };

            return response.dump();

        } catch (const json::parse_error& e) {
            spdlog::error("Broadcast handler JSON parse error: {}", e.what());
            json error_json = {
                {"error", "Bad Request"},
                {"message", "Invalid JSON in request body"}
            };
            return error_json.dump();
        } catch (const std::exception& e) {
            spdlog::error("Broadcast handler error: {}", e.what());
            json error_json = {
                {"error", "Internal Server Error"},
                {"message", e.what()}
            };
            return error_json.dump();
        }
    });

    // POST /api/admin/kick - Disconnect a player
    admin_server.register_handler("/api/admin/kick", [&mud_server]([[maybe_unused]] const std::string& path, const std::string& body) -> std::string {
        spdlog::info("Received kick request");

        try {
            auto request = json::parse(body);

            if (!request.contains("player_name")) {
                json error_json = {
                    {"error", "Bad Request"},
                    {"message", "Missing required field: player_name"}
                };
                return error_json.dump();
            }

            std::string player_name = request["player_name"].get<std::string>();
            std::string reason = request.value("reason", "Disconnected by administrator");

            // Check if player exists
            auto player = mud_server.find_player(player_name);
            if (!player) {
                json error_json = {
                    {"error", "Not Found"},
                    {"message", fmt::format("Player '{}' not found online", player_name)}
                };
                return error_json.dump();
            }

            // Kick the player
            mud_server.kick_player(player_name, reason);

            spdlog::info("Player {} kicked. Reason: {}", player_name, reason);

            json response = {
                {"success", true},
                {"message", fmt::format("Player '{}' has been disconnected", player_name)},
                {"reason", reason}
            };

            return response.dump();

        } catch (const json::parse_error& e) {
            spdlog::error("Kick handler JSON parse error: {}", e.what());
            json error_json = {
                {"error", "Bad Request"},
                {"message", "Invalid JSON in request body"}
            };
            return error_json.dump();
        } catch (const std::exception& e) {
            spdlog::error("Kick handler error: {}", e.what());
            json error_json = {
                {"error", "Internal Server Error"},
                {"message", e.what()}
            };
            return error_json.dump();
        }
    });

    // GET /api/admin/stats - Server statistics
    admin_server.register_handler("/api/admin/stats", [&mud_server]([[maybe_unused]] const std::string& path, [[maybe_unused]] const std::string& body) -> std::string {
        spdlog::info("Received stats request");

        try {
            const auto& stats = mud_server.stats();

            json response = {
                {"success", true},
                {"stats", {
                    {"uptime_seconds", stats.uptime().count()},
                    {"total_connections", stats.total_connections.load()},
                    {"current_connections", stats.current_connections.load()},
                    {"peak_connections", stats.peak_connections.load()},
                    {"total_commands", stats.total_commands.load()},
                    {"failed_commands", stats.failed_commands.load()},
                    {"total_logins", stats.total_logins.load()},
                    {"failed_logins", stats.failed_logins.load()},
                    {"commands_per_second", stats.commands_per_second()}
                }},
                {"server", {
                    {"name", mud_server.config().mud_name},
                    {"port", mud_server.config().port},
                    {"tls_port", mud_server.config().tls_port},
                    {"maintenance_mode", mud_server.is_maintenance_mode()},
                    {"running", mud_server.is_running()}
                }}
            };

            return response.dump();

        } catch (const std::exception& e) {
            spdlog::error("Stats handler error: {}", e.what());
            json error_json = {
                {"error", "Internal Server Error"},
                {"message", e.what()}
            };
            return error_json.dump();
        }
    });

    spdlog::info("Registered player and admin handlers with admin server");
}

}  // namespace fierymud
