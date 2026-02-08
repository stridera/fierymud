#include "lua_quest.hpp"

#include "core/actor.hpp"
#include "quests/quest_manager.hpp"

#define SOL_ALL_SAFETIES_ON 1
#include <nlohmann/json.hpp>
#include <sol/sol.hpp>
#include <spdlog/spdlog.h>

namespace FieryMUD {

namespace {

// Convert QuestStatus enum to string for Lua
std::string status_to_string(QuestStatus status) {
    switch (status) {
    case QuestStatus::Available:
        return "AVAILABLE";
    case QuestStatus::InProgress:
        return "IN_PROGRESS";
    case QuestStatus::Completed:
        return "COMPLETED";
    case QuestStatus::Failed:
        return "FAILED";
    case QuestStatus::Abandoned:
        return "ABANDONED";
    default:
        return "NONE";
    }
}

} // anonymous namespace

void register_quest_bindings(sol::state &lua) {
    // Create quest table/namespace
    auto quest_table = lua.create_named_table("quest");

    // quest.start(actor, zone_id, quest_id) - Start a quest for an actor
    quest_table["start"] = [](Actor *actor, int zone_id, int quest_id) -> bool {
        if (!actor) {
            spdlog::warn("quest.start: null actor");
            return false;
        }

        auto &manager = QuestManager::instance();
        auto result = manager.start_quest(*actor, zone_id, quest_id);
        if (!result) {
            spdlog::debug("quest.start failed: {}", result.error().message);
            return false;
        }
        return true;
    };

    // quest.complete(actor, zone_id, quest_id) - Complete a quest
    quest_table["complete"] = [](Actor *actor, int zone_id, int quest_id) -> bool {
        if (!actor) {
            spdlog::warn("quest.complete: null actor");
            return false;
        }

        auto &manager = QuestManager::instance();
        auto result = manager.complete_quest(*actor, zone_id, quest_id);
        if (!result) {
            spdlog::debug("quest.complete failed: {}", result.error().message);
            return false;
        }
        return true;
    };

    // quest.abandon(actor, zone_id, quest_id) - Abandon a quest
    quest_table["abandon"] = [](Actor *actor, int zone_id, int quest_id) -> bool {
        if (!actor) {
            spdlog::warn("quest.abandon: null actor");
            return false;
        }

        auto &manager = QuestManager::instance();
        auto result = manager.abandon_quest(*actor, zone_id, quest_id);
        if (!result) {
            spdlog::debug("quest.abandon failed: {}", result.error().message);
            return false;
        }
        return true;
    };

    // quest.status(actor, zone_id, quest_id) - Get quest status as string
    quest_table["status"] = [](Actor *actor, int zone_id, int quest_id) -> std::string {
        if (!actor) {
            return "NONE";
        }

        auto &manager = QuestManager::instance();
        auto status = manager.get_quest_status(*actor, zone_id, quest_id);
        return status_to_string(status);
    };

    // quest.has_quest(actor, zone_id, quest_id) - Check if actor has quest in progress
    quest_table["has_quest"] = [](Actor *actor, int zone_id, int quest_id) -> bool {
        if (!actor) {
            return false;
        }

        auto &manager = QuestManager::instance();
        auto status = manager.get_quest_status(*actor, zone_id, quest_id);
        return status == QuestStatus::InProgress;
    };

    // quest.is_available(actor, zone_id, quest_id) - Check if quest is available
    quest_table["is_available"] = [](Actor *actor, int zone_id, int quest_id) -> bool {
        if (!actor) {
            return false;
        }

        auto &manager = QuestManager::instance();
        return manager.is_quest_available(*actor, zone_id, quest_id);
    };

    // quest.is_completed(actor, zone_id, quest_id) - Check if completed before
    quest_table["is_completed"] = [](Actor *actor, int zone_id, int quest_id) -> bool {
        if (!actor) {
            return false;
        }

        auto &manager = QuestManager::instance();
        auto status = manager.get_quest_status(*actor, zone_id, quest_id);
        return status == QuestStatus::Completed;
    };

    // quest.advance_objective(actor, zone_id, quest_id, objective_id, count)
    quest_table["advance_objective"] = [](Actor *actor, int zone_id, int quest_id, int objective_id,
                                          sol::optional<int> count) -> bool {
        if (!actor) {
            spdlog::warn("quest.advance_objective: null actor");
            return false;
        }

        auto &manager = QuestManager::instance();
        int advance_count = count.value_or(1);
        auto result = manager.advance_objective(*actor, zone_id, quest_id, objective_id, advance_count);
        if (!result) {
            spdlog::debug("quest.advance_objective failed: {}", result.error().message);
            return false;
        }
        return true;
    };

    // quest.set_variable(actor, zone_id, quest_id, name, value)
    quest_table["set_variable"] = [](Actor *actor, int zone_id, int quest_id, const std::string &name,
                                     sol::object value) -> bool {
        if (!actor) {
            spdlog::warn("quest.set_variable: null actor");
            return false;
        }

        auto &manager = QuestManager::instance();

        // Convert Lua value to appropriate C++ type
        nlohmann::json json_value;
        if (value.is<bool>()) {
            json_value = value.as<bool>();
        } else if (value.is<int>()) {
            json_value = value.as<int>();
        } else if (value.is<double>()) {
            json_value = value.as<double>();
        } else if (value.is<std::string>()) {
            json_value = value.as<std::string>();
        } else {
            json_value = nullptr;
        }

        auto result = manager.set_quest_variable(*actor, zone_id, quest_id, name, json_value);
        if (!result) {
            spdlog::debug("quest.set_variable failed: {}", result.error().message);
            return false;
        }
        return true;
    };

    // quest.get_variable(actor, zone_id, quest_id, name)
    quest_table["get_variable"] = [&lua](Actor *actor, int zone_id, int quest_id,
                                         const std::string &name) -> sol::object {
        if (!actor) {
            return sol::nil;
        }

        auto &manager = QuestManager::instance();
        auto result = manager.get_quest_variable(*actor, zone_id, quest_id, name);
        if (!result) {
            return sol::nil;
        }

        const auto &json_value = *result;

        // Convert JSON to Lua type
        if (json_value.is_boolean()) {
            return sol::make_object(lua, json_value.get<bool>());
        } else if (json_value.is_number_integer()) {
            return sol::make_object(lua, json_value.get<int>());
        } else if (json_value.is_number_float()) {
            return sol::make_object(lua, json_value.get<double>());
        } else if (json_value.is_string()) {
            return sol::make_object(lua, json_value.get<std::string>());
        }

        return sol::nil;
    };

    spdlog::debug("Registered quest Lua bindings");
}

} // namespace FieryMUD
