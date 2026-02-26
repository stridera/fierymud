#include "trigger_handler.hpp"

#include <fmt/format.h>
#include <magic_enum/magic_enum.hpp>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

#include "core/actor.hpp"
#include "core/mobile.hpp"
#include "core/player.hpp"
#include "database/connection_pool.hpp"
#include "database/trigger_queries.hpp"
#include "scripting/script_context.hpp"
#include "scripting/trigger_manager.hpp"
#include "scripting/triggers/trigger_data.hpp"
#include "server/mud_server.hpp"
#include "session_handler.hpp"
#include "world/room.hpp"
#include "world/world_manager.hpp"

using json = nlohmann::json;

namespace fierymud {

namespace {

/// Parse zoneId from path like "/api/admin/triggers/zone/30"
int parse_zone_from_path(const std::string &path, const std::string &prefix) {
    if (!path.starts_with(prefix)) {
        return -1;
    }
    try {
        return std::stoi(path.substr(prefix.size()));
    } catch (...) {
        return -1;
    }
}

/// Parse zoneId/id from path like "/api/admin/triggers/30/1"
bool parse_trigger_id_from_path(const std::string &path, const std::string &prefix, int &zone_id, int &trigger_id) {
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
        // Remove trailing path components like "/fire"
        auto id_str = rest.substr(slash + 1);
        auto next_slash = id_str.find('/');
        if (next_slash != std::string::npos) {
            id_str = id_str.substr(0, next_slash);
        }
        trigger_id = std::stoi(id_str);
        return true;
    } catch (...) {
        return false;
    }
}

/// Serialize a trigger to JSON
json trigger_to_json(const FieryMUD::TriggerDataPtr &trigger) {
    json obj = {{"id", trigger->id},
                {"name", trigger->name},
                {"attach_type", std::string(magic_enum::enum_name(trigger->attach_type))},
                {"flags", trigger->flags_string()},
                {"num_args", trigger->num_args},
                {"numeric_arg", trigger->numeric_arg}};

    if (trigger->mob_id) {
        obj["mob_zone_id"] = trigger->mob_id->zone_id();
        obj["mob_id"] = trigger->mob_id->local_id();
    }
    if (trigger->object_id) {
        obj["object_zone_id"] = trigger->object_id->zone_id();
        obj["object_id"] = trigger->object_id->local_id();
    }
    if (trigger->zone_id) {
        obj["zone_id"] = *trigger->zone_id;
    }

    return obj;
}

/// Serialize a trigger error to JSON
json error_to_json(const TriggerQueries::ScriptErrorEntry &error) {
    json obj = {{"id", error.id},
                {"zone_id", error.zone_id},
                {"trigger_id", error.trigger_id},
                {"trigger_name", error.trigger_name},
                {"error_type", error.error_type},
                {"error_message", error.error_message},
                {"occurred_at", error.occurred_at}};
    if (error.script_line) {
        obj["script_line"] = *error.script_line;
    }
    return obj;
}

} // namespace

void register_trigger_handlers(AdminServer &admin_server, ModernMUDServer &mud_server) {
    // GET /api/admin/triggers/errors - Triggers needing review
    admin_server.register_handler(
        "/api/admin/triggers/errors",
        []([[maybe_unused]] const std::string &path, [[maybe_unused]] const std::string &body) -> std::string {
            // Check for /recent subpath
            if (path.find("/recent") != std::string::npos) {
                auto result = ConnectionPool::instance().execute(
                    [](pqxx::work &txn) { return TriggerQueries::get_recent_script_errors(txn, 50); });

                if (!result) {
                    return json({{"error", "Database Error"}, {"message", result.error().message}}).dump();
                }

                json errors = json::array();
                for (const auto &entry : *result) {
                    errors.push_back(error_to_json(entry));
                }
                return json({{"success", true}, {"count", errors.size()}, {"errors", errors}}).dump();
            }

            // Default: get triggers needing review
            auto result = ConnectionPool::instance().execute(
                [](pqxx::work &txn) { return TriggerQueries::get_triggers_needing_review(txn); });

            if (!result) {
                return json({{"error", "Database Error"}, {"message", result.error().message}}).dump();
            }

            json triggers = json::array();
            for (const auto &trigger : *result) {
                triggers.push_back(trigger_to_json(trigger));
            }
            return json({{"success", true}, {"count", triggers.size()}, {"triggers", triggers}}).dump();
        });

    // GET /api/admin/triggers/stats - Execution statistics
    admin_server.register_handler(
        "/api/admin/triggers/stats",
        []([[maybe_unused]] const std::string &path, [[maybe_unused]] const std::string &body) -> std::string {
            auto &tm = FieryMUD::TriggerManager::instance();
            const auto &stats = tm.stats();

            return json({{"success", true},
                         {"stats",
                          {{"total_executions", stats.total_executions},
                           {"successful_executions", stats.successful_executions},
                           {"halted_executions", stats.halted_executions},
                           {"failed_executions", stats.failed_executions},
                           {"yielded_executions", stats.yielded_executions},
                           {"total_triggers_cached", tm.trigger_count()}}}})
                .dump();
        });

    // GET /api/admin/triggers/zone/* - List triggers for a zone
    admin_server.register_handler(
        "/api/admin/triggers/zone",
        []([[maybe_unused]] const std::string &path, [[maybe_unused]] const std::string &body) -> std::string {
            int zone_id = parse_zone_from_path(path, "/api/admin/triggers/zone/");
            if (zone_id < 0) {
                return json({{"error", "Bad Request"}, {"message", "Path: /api/admin/triggers/zone/{zoneId}"}}).dump();
            }

            auto result = ConnectionPool::instance().execute(
                [zone_id](pqxx::work &txn) { return TriggerQueries::load_triggers_for_zone(txn, zone_id); });

            if (!result) {
                return json({{"error", "Database Error"}, {"message", result.error().message}}).dump();
            }

            json triggers = json::array();
            for (const auto &trigger : *result) {
                triggers.push_back(trigger_to_json(trigger));
            }

            return json({{"success", true}, {"zone_id", zone_id}, {"count", triggers.size()}, {"triggers", triggers}})
                .dump();
        });

    // GET /api/admin/triggers/{zoneId}/{id} and POST .../fire
    admin_server.register_handler(
        "/api/admin/triggers", [&mud_server](const std::string &path, const std::string &body) -> std::string {
            // Route sub-paths that are handled by other registered handlers
            if (path.find("/api/admin/triggers/errors") == 0 || path.find("/api/admin/triggers/stats") == 0 ||
                path.find("/api/admin/triggers/zone") == 0 || path.find("/api/admin/triggers/reload") == 0) {
                return json({{"error", "Internal routing error"}}).dump();
            }

            int zone_id = 0;
            int trigger_id = 0;
            if (!parse_trigger_id_from_path(path, "/api/admin/triggers/", zone_id, trigger_id)) {
                return json({{"error", "Bad Request"}, {"message", "Path: /api/admin/triggers/{zoneId}/{id}"}}).dump();
            }

            // Check if this is a /fire request
            bool is_fire = path.find("/fire") != std::string::npos;

            if (is_fire) {
                // POST /api/admin/triggers/{zoneId}/{id}/fire
                spdlog::info("Received trigger fire request for {}:{}", zone_id, trigger_id);

                try {
                    auto request = json::parse(body);
                    std::string actor_name = request.value("actor", "");
                    std::string target_name = request.value("target", "");
                    std::string speech = request.value("speech", "");

                    // Find the trigger
                    EntityId trig_id{static_cast<uint32_t>(zone_id), static_cast<uint32_t>(trigger_id)};
                    auto trigger = FieryMUD::TriggerManager::instance().find_trigger_by_id(trig_id);
                    if (!trigger) {
                        return json({{"error", "Not Found"},
                                     {"message", fmt::format("Trigger {}:{} not found in cache", zone_id, trigger_id)}})
                            .dump();
                    }

                    // Find actor
                    std::shared_ptr<Actor> actor;
                    if (!actor_name.empty()) {
                        auto player = mud_server.find_player(actor_name);
                        if (player) {
                            actor = std::static_pointer_cast<Actor>(player);
                        }
                    }

                    // Find the owner (mob attached to the trigger)
                    std::shared_ptr<Actor> owner;
                    if (!target_name.empty()) {
                        // Look for the named target in the world
                        auto mob = WorldManager::instance().find_mobile(target_name);
                        if (mob) {
                            owner = std::static_pointer_cast<Actor>(mob);
                        }
                    } else if (trigger->mob_id) {
                        // Use first spawned instance of the attached mob
                        auto mob = WorldManager::instance().find_mobile(trigger->name);
                        if (mob) {
                            owner = std::static_pointer_cast<Actor>(mob);
                        }
                    }

                    // Build script context
                    auto ctx_builder = FieryMUD::ScriptContext::Builder{};
                    ctx_builder.set_trigger(trigger);

                    if (owner) {
                        ctx_builder.set_owner(owner);
                        if (auto room = owner->current_room()) {
                            ctx_builder.set_room(room);
                        }
                    }
                    if (actor) {
                        ctx_builder.set_actor(actor);
                    }
                    if (!speech.empty()) {
                        ctx_builder.set_speech(speech);
                    }

                    auto context = ctx_builder.build();

                    // Clear output queues to capture trigger output
                    if (actor) {
                        if (auto player = std::dynamic_pointer_cast<Player>(actor)) {
                            player->clear_output_queue();
                        }
                    }

                    // Execute the trigger
                    auto result = FieryMUD::TriggerManager::instance().debug_execute_trigger(trigger, context);

                    // Capture output
                    json actor_output = json::array();
                    if (actor) {
                        if (auto player = std::dynamic_pointer_cast<Player>(actor)) {
                            for (const auto &line : player->get_output_queue()) {
                                actor_output.push_back(line);
                            }
                        }
                    }

                    std::string result_str;
                    switch (result) {
                    case FieryMUD::TriggerResult::Continue:
                        result_str = "Continue";
                        break;
                    case FieryMUD::TriggerResult::Halt:
                        result_str = "Halt";
                        break;
                    case FieryMUD::TriggerResult::Error:
                        result_str = "Error";
                        break;
                    }

                    json response = {{"success", result != FieryMUD::TriggerResult::Error},
                                     {"result", result_str},
                                     {"trigger_name", trigger->name},
                                     {"actor_output", actor_output}};

                    if (result == FieryMUD::TriggerResult::Error) {
                        response["error_message"] = std::string(FieryMUD::TriggerManager::instance().last_error());
                    }

                    return response.dump();

                } catch (const json::parse_error &e) {
                    return json({{"error", "Bad Request"}, {"message", "Invalid JSON"}}).dump();
                } catch (const std::exception &e) {
                    spdlog::error("Trigger fire error: {}", e.what());
                    return json({{"error", "Internal Server Error"}, {"message", e.what()}}).dump();
                }
            }

            // GET /api/admin/triggers/{zoneId}/{id} - Trigger details
            auto result = ConnectionPool::instance().execute([zone_id, trigger_id](pqxx::work &txn) {
                return TriggerQueries::load_trigger_by_id(txn, zone_id, trigger_id);
            });

            if (!result) {
                return json({{"error", "Not Found"},
                             {"message",
                              fmt::format("Trigger {}:{} not found: {}", zone_id, trigger_id, result.error().message)}})
                    .dump();
            }

            auto &trigger = *result;
            json response = trigger_to_json(trigger);
            response["success"] = true;
            response["commands"] = trigger->commands;
            response["variables"] = trigger->variables;

            // Get recent error log
            auto errors_result = ConnectionPool::instance().execute([zone_id, trigger_id](pqxx::work &txn) {
                return TriggerQueries::get_error_log_for_trigger(txn, zone_id, trigger_id, 5);
            });

            if (errors_result) {
                json errors = json::array();
                for (const auto &entry : *errors_result) {
                    errors.push_back(error_to_json(entry));
                }
                response["recent_errors"] = errors;
            }

            return response.dump();
        });

    // POST /api/admin/triggers/reload/* - Reload zone triggers
    admin_server.register_handler(
        "/api/admin/triggers/reload",
        [](const std::string &path, [[maybe_unused]] const std::string &body) -> std::string {
            int zone_id = parse_zone_from_path(path, "/api/admin/triggers/reload/");
            if (zone_id < 0) {
                return json({{"error", "Bad Request"}, {"message", "Path: /api/admin/triggers/reload/{zoneId}"}})
                    .dump();
            }

            spdlog::info("Reloading triggers for zone {}", zone_id);

            auto result = FieryMUD::TriggerManager::instance().reload_zone_triggers(static_cast<uint32_t>(zone_id));

            if (result) {
                return json({{"success", true},
                             {"message", fmt::format("Reloaded {} triggers for zone {}", *result, zone_id)},
                             {"zone_id", zone_id},
                             {"count", *result}})
                    .dump();
            } else {
                return json({{"error", "Failed"}, {"message", result.error()}, {"zone_id", zone_id}}).dump();
            }
        });

    spdlog::info("Registered trigger handlers with admin server");
}

} // namespace fierymud
