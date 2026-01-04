#include "database/quest_queries.hpp"
#include "core/logging.hpp"
#include <fmt/format.h>
#include <fmt/chrono.h>

namespace QuestQueries {

// ============================================================================
// Helper Functions Implementation
// ============================================================================

QuestObjectiveType parse_objective_type(std::string_view type_str) {
    if (type_str == "KILL_MOB") return QuestObjectiveType::KILL_MOB;
    if (type_str == "COLLECT_ITEM") return QuestObjectiveType::COLLECT_ITEM;
    if (type_str == "DELIVER_ITEM") return QuestObjectiveType::DELIVER_ITEM;
    if (type_str == "VISIT_ROOM") return QuestObjectiveType::VISIT_ROOM;
    if (type_str == "TALK_TO_NPC") return QuestObjectiveType::TALK_TO_NPC;
    if (type_str == "USE_SKILL") return QuestObjectiveType::USE_SKILL;
    if (type_str == "CUSTOM_LUA") return QuestObjectiveType::CUSTOM_LUA;
    return QuestObjectiveType::KILL_MOB; // Default fallback
}

QuestRewardType parse_reward_type(std::string_view type_str) {
    if (type_str == "EXPERIENCE") return QuestRewardType::EXPERIENCE;
    if (type_str == "GOLD") return QuestRewardType::GOLD;
    if (type_str == "ITEM") return QuestRewardType::ITEM;
    if (type_str == "ABILITY") return QuestRewardType::ABILITY;
    return QuestRewardType::EXPERIENCE; // Default fallback
}

QuestStatus parse_quest_status(std::string_view status_str) {
    if (status_str == "AVAILABLE") return QuestStatus::AVAILABLE;
    if (status_str == "IN_PROGRESS") return QuestStatus::IN_PROGRESS;
    if (status_str == "COMPLETED") return QuestStatus::COMPLETED;
    if (status_str == "FAILED") return QuestStatus::FAILED;
    if (status_str == "ABANDONED") return QuestStatus::ABANDONED;
    return QuestStatus::AVAILABLE; // Default fallback
}

std::string_view objective_type_to_string(QuestObjectiveType type) {
    switch (type) {
        case QuestObjectiveType::KILL_MOB: return "KILL_MOB";
        case QuestObjectiveType::COLLECT_ITEM: return "COLLECT_ITEM";
        case QuestObjectiveType::DELIVER_ITEM: return "DELIVER_ITEM";
        case QuestObjectiveType::VISIT_ROOM: return "VISIT_ROOM";
        case QuestObjectiveType::TALK_TO_NPC: return "TALK_TO_NPC";
        case QuestObjectiveType::USE_SKILL: return "USE_SKILL";
        case QuestObjectiveType::CUSTOM_LUA: return "CUSTOM_LUA";
    }
    return "KILL_MOB";
}

std::string_view reward_type_to_string(QuestRewardType type) {
    switch (type) {
        case QuestRewardType::EXPERIENCE: return "EXPERIENCE";
        case QuestRewardType::GOLD: return "GOLD";
        case QuestRewardType::ITEM: return "ITEM";
        case QuestRewardType::ABILITY: return "ABILITY";
    }
    return "EXPERIENCE";
}

std::string_view quest_status_to_string(QuestStatus status) {
    switch (status) {
        case QuestStatus::AVAILABLE: return "AVAILABLE";
        case QuestStatus::IN_PROGRESS: return "IN_PROGRESS";
        case QuestStatus::COMPLETED: return "COMPLETED";
        case QuestStatus::FAILED: return "FAILED";
        case QuestStatus::ABANDONED: return "ABANDONED";
    }
    return "AVAILABLE";
}

QuestTriggerType parse_trigger_type(std::string_view type_str) {
    if (type_str == "MOB") return QuestTriggerType::MOB;
    if (type_str == "LEVEL") return QuestTriggerType::LEVEL;
    if (type_str == "ITEM") return QuestTriggerType::ITEM;
    if (type_str == "ROOM") return QuestTriggerType::ROOM;
    if (type_str == "SKILL") return QuestTriggerType::SKILL;
    if (type_str == "EVENT") return QuestTriggerType::EVENT;
    if (type_str == "AUTO") return QuestTriggerType::AUTO;
    if (type_str == "MANUAL") return QuestTriggerType::MANUAL;
    return QuestTriggerType::MOB; // Default fallback
}

std::string_view trigger_type_to_string(QuestTriggerType type) {
    switch (type) {
        case QuestTriggerType::MOB: return "MOB";
        case QuestTriggerType::LEVEL: return "LEVEL";
        case QuestTriggerType::ITEM: return "ITEM";
        case QuestTriggerType::ROOM: return "ROOM";
        case QuestTriggerType::SKILL: return "SKILL";
        case QuestTriggerType::EVENT: return "EVENT";
        case QuestTriggerType::AUTO: return "AUTO";
        case QuestTriggerType::MANUAL: return "MANUAL";
    }
    return "MOB";
}

// ============================================================================
// Quest Definition Queries
// ============================================================================

Result<QuestData> load_quest(pqxx::work& txn, int zone_id, int quest_id) {
    auto logger = Log::database();
    logger->debug("Loading quest {}:{} from database", zone_id, quest_id);

    try {
        // Load main quest data
        auto quest_result = txn.exec_params(R"(
            SELECT zone_id, id, name, description,
                   min_level, max_level, repeatable, hidden,
                   trigger_type, trigger_level,
                   trigger_item_zone_id, trigger_item_id,
                   trigger_room_zone_id, trigger_room_id,
                   trigger_ability_id, trigger_event_id,
                   giver_mob_zone_id, giver_mob_id,
                   completer_mob_zone_id, completer_mob_id
            FROM "Quest"
            WHERE zone_id = $1 AND id = $2
        )", zone_id, quest_id);

        if (quest_result.empty()) {
            logger->debug("Quest {}:{} not found", zone_id, quest_id);
            return std::unexpected(Errors::NotFound(
                fmt::format("Quest {}:{}", zone_id, quest_id)));
        }

        const auto& row = quest_result[0];
        QuestData quest;
        quest.id = EntityId(row["zone_id"].as<int>(), row["id"].as<int>());
        quest.name = row["name"].as<std::string>();
        quest.description = row["description"].is_null() ? "" : row["description"].as<std::string>();
        quest.min_level = row["min_level"].is_null() ? 1 : row["min_level"].as<int>();
        quest.max_level = row["max_level"].is_null() ? 100 : row["max_level"].as<int>();
        quest.repeatable = row["repeatable"].as<bool>();
        quest.hidden = row["hidden"].as<bool>();

        // Load trigger configuration
        quest.trigger_type = parse_trigger_type(
            row["trigger_type"].is_null() ? "MOB" : row["trigger_type"].as<std::string>());
        if (!row["trigger_level"].is_null()) {
            quest.trigger_level = row["trigger_level"].as<int>();
        }
        if (!row["trigger_item_zone_id"].is_null() && !row["trigger_item_id"].is_null()) {
            quest.trigger_item = EntityId(
                row["trigger_item_zone_id"].as<int>(),
                row["trigger_item_id"].as<int>());
        }
        if (!row["trigger_room_zone_id"].is_null() && !row["trigger_room_id"].is_null()) {
            quest.trigger_room = EntityId(
                row["trigger_room_zone_id"].as<int>(),
                row["trigger_room_id"].as<int>());
        }
        if (!row["trigger_ability_id"].is_null()) {
            quest.trigger_ability_id = row["trigger_ability_id"].as<int>();
        }
        if (!row["trigger_event_id"].is_null()) {
            quest.trigger_event_id = row["trigger_event_id"].as<int>();
        }

        if (!row["giver_mob_zone_id"].is_null() && !row["giver_mob_id"].is_null()) {
            quest.giver_mob = EntityId(
                row["giver_mob_zone_id"].as<int>(),
                row["giver_mob_id"].as<int>());
        }
        if (!row["completer_mob_zone_id"].is_null() && !row["completer_mob_id"].is_null()) {
            quest.completer_mob = EntityId(
                row["completer_mob_zone_id"].as<int>(),
                row["completer_mob_id"].as<int>());
        }

        // Load phases
        auto phases_result = txn.exec_params(R"(
            SELECT id, name, description, "order"
            FROM "QuestPhase"
            WHERE quest_zone_id = $1 AND quest_id = $2
            ORDER BY "order" ASC
        )", zone_id, quest_id);

        for (const auto& phase_row : phases_result) {
            QuestPhase phase;
            phase.id = phase_row["id"].as<int>();
            phase.name = phase_row["name"].as<std::string>();
            phase.description = phase_row["description"].is_null() ? "" : phase_row["description"].as<std::string>();
            phase.order = phase_row["order"].as<int>();

            // Load objectives for this phase
            auto objectives_result = txn.exec_params(R"(
                SELECT id, objective_type, player_description, internal_note,
                       show_progress, required_count,
                       target_mob_zone_id, target_mob_id,
                       target_object_zone_id, target_object_id,
                       target_room_zone_id, target_room_id,
                       target_ability_id,
                       deliver_to_mob_zone_id, deliver_to_mob_id,
                       lua_expression
                FROM "QuestObjective"
                WHERE quest_zone_id = $1 AND quest_id = $2 AND phase_id = $3
                ORDER BY id ASC
            )", zone_id, quest_id, phase.id);

            for (const auto& obj_row : objectives_result) {
                QuestObjective obj;
                obj.phase_id = phase.id;
                obj.id = obj_row["id"].as<int>();
                obj.type = parse_objective_type(obj_row["objective_type"].as<std::string>());
                obj.player_description = obj_row["player_description"].as<std::string>();
                obj.internal_note = obj_row["internal_note"].is_null() ? "" : obj_row["internal_note"].as<std::string>();
                obj.show_progress = obj_row["show_progress"].as<bool>();
                obj.required_count = obj_row["required_count"].as<int>();

                if (!obj_row["target_mob_zone_id"].is_null() && !obj_row["target_mob_id"].is_null()) {
                    obj.target_mob = EntityId(
                        obj_row["target_mob_zone_id"].as<int>(),
                        obj_row["target_mob_id"].as<int>());
                }
                if (!obj_row["target_object_zone_id"].is_null() && !obj_row["target_object_id"].is_null()) {
                    obj.target_object = EntityId(
                        obj_row["target_object_zone_id"].as<int>(),
                        obj_row["target_object_id"].as<int>());
                }
                if (!obj_row["target_room_zone_id"].is_null() && !obj_row["target_room_id"].is_null()) {
                    obj.target_room = EntityId(
                        obj_row["target_room_zone_id"].as<int>(),
                        obj_row["target_room_id"].as<int>());
                }
                if (!obj_row["target_ability_id"].is_null()) {
                    obj.target_ability_id = obj_row["target_ability_id"].as<int>();
                }
                if (!obj_row["deliver_to_mob_zone_id"].is_null() && !obj_row["deliver_to_mob_id"].is_null()) {
                    obj.deliver_to_mob = EntityId(
                        obj_row["deliver_to_mob_zone_id"].as<int>(),
                        obj_row["deliver_to_mob_id"].as<int>());
                }
                obj.lua_expression = obj_row["lua_expression"].is_null() ? "" : obj_row["lua_expression"].as<std::string>();

                phase.objectives.push_back(std::move(obj));
            }

            quest.phases.push_back(std::move(phase));
        }

        // Load rewards
        auto rewards_result = txn.exec_params(R"(
            SELECT id, reward_type, amount,
                   object_zone_id, object_id, ability_id, choice_group
            FROM "QuestReward"
            WHERE quest_zone_id = $1 AND quest_id = $2
            ORDER BY id ASC
        )", zone_id, quest_id);

        for (const auto& reward_row : rewards_result) {
            QuestReward reward;
            reward.id = reward_row["id"].as<int>();
            reward.type = parse_reward_type(reward_row["reward_type"].as<std::string>());
            if (!reward_row["amount"].is_null()) {
                reward.amount = reward_row["amount"].as<int>();
            }
            if (!reward_row["object_zone_id"].is_null() && !reward_row["object_id"].is_null()) {
                reward.object = EntityId(
                    reward_row["object_zone_id"].as<int>(),
                    reward_row["object_id"].as<int>());
            }
            if (!reward_row["ability_id"].is_null()) {
                reward.ability_id = reward_row["ability_id"].as<int>();
            }
            if (!reward_row["choice_group"].is_null()) {
                reward.choice_group = reward_row["choice_group"].as<int>();
            }

            quest.rewards.push_back(std::move(reward));
        }

        // Load prerequisites
        auto prereqs_result = txn.exec_params(R"(
            SELECT id, prerequisite_quest_zone_id, prerequisite_quest_id
            FROM "QuestPrerequisite"
            WHERE quest_zone_id = $1 AND quest_id = $2
        )", zone_id, quest_id);

        for (const auto& prereq_row : prereqs_result) {
            QuestPrerequisite prereq;
            prereq.id = prereq_row["id"].as<int>();
            prereq.prerequisite_quest = EntityId(
                prereq_row["prerequisite_quest_zone_id"].as<int>(),
                prereq_row["prerequisite_quest_id"].as<int>());
            quest.prerequisites.push_back(std::move(prereq));
        }

        logger->debug("Loaded quest {}:{} with {} phases, {} rewards, {} prerequisites",
            zone_id, quest_id, quest.phases.size(), quest.rewards.size(), quest.prerequisites.size());

        return quest;

    } catch (const std::exception& e) {
        logger->error("Database error loading quest {}:{}: {}", zone_id, quest_id, e.what());
        return std::unexpected(Errors::DatabaseError(e.what()));
    }
}

Result<std::vector<QuestData>> load_quests_in_zone(pqxx::work& txn, int zone_id) {
    auto logger = Log::database();
    logger->debug("Loading all quests in zone {}", zone_id);

    try {
        auto result = txn.exec_params(R"(
            SELECT id FROM "Quest" WHERE zone_id = $1 ORDER BY id ASC
        )", zone_id);

        std::vector<QuestData> quests;
        quests.reserve(result.size());

        for (const auto& row : result) {
            auto quest_result = load_quest(txn, zone_id, row["id"].as<int>());
            if (quest_result) {
                quests.push_back(std::move(*quest_result));
            }
        }

        logger->debug("Loaded {} quests from zone {}", quests.size(), zone_id);
        return quests;

    } catch (const std::exception& e) {
        logger->error("Database error loading quests in zone {}: {}", zone_id, e.what());
        return std::unexpected(Errors::DatabaseError(e.what()));
    }
}

Result<std::vector<QuestData>> load_available_quests(
    pqxx::work& txn,
    std::string_view character_id,
    int level) {

    auto logger = Log::database();
    logger->debug("Loading available quests for character {} at level {}", character_id, level);

    try {
        // Find quests the character can accept:
        // - Within level range
        // - Not hidden
        // - Not currently in progress
        // - All prerequisites completed
        auto result = txn.exec_params(R"(
            SELECT q.zone_id, q.id
            FROM "Quest" q
            WHERE q.hidden = false
              AND q.min_level <= $2
              AND q.max_level >= $2
              AND NOT EXISTS (
                  SELECT 1 FROM "CharacterQuest" cq
                  WHERE cq.character_id = $1
                    AND cq.quest_zone_id = q.zone_id
                    AND cq.quest_id = q.id
                    AND cq.status = 'IN_PROGRESS'
              )
              AND NOT EXISTS (
                  SELECT 1 FROM "QuestPrerequisite" qp
                  WHERE qp.quest_zone_id = q.zone_id
                    AND qp.quest_id = q.id
                    AND NOT EXISTS (
                        SELECT 1 FROM "CharacterQuest" cq2
                        WHERE cq2.character_id = $1
                          AND cq2.quest_zone_id = qp.prerequisite_quest_zone_id
                          AND cq2.quest_id = qp.prerequisite_quest_id
                          AND cq2.status = 'COMPLETED'
                    )
              )
            ORDER BY q.zone_id, q.id
        )", std::string(character_id), level);

        std::vector<QuestData> quests;
        for (const auto& row : result) {
            auto quest_result = load_quest(txn, row["zone_id"].as<int>(), row["id"].as<int>());
            if (quest_result) {
                quests.push_back(std::move(*quest_result));
            }
        }

        logger->debug("Found {} available quests for character {}", quests.size(), character_id);
        return quests;

    } catch (const std::exception& e) {
        logger->error("Database error loading available quests: {}", e.what());
        return std::unexpected(Errors::DatabaseError(e.what()));
    }
}

Result<bool> can_accept_quest(
    pqxx::work& txn,
    std::string_view character_id,
    int zone_id, int quest_id,
    int level) {

    auto logger = Log::database();

    try {
        // Load the quest to check level requirements
        auto quest_result = txn.exec_params(R"(
            SELECT min_level, max_level, hidden, repeatable
            FROM "Quest"
            WHERE zone_id = $1 AND id = $2
        )", zone_id, quest_id);

        if (quest_result.empty()) {
            return std::unexpected(Errors::NotFound(
                fmt::format("Quest {}:{}", zone_id, quest_id)));
        }

        const auto& quest_row = quest_result[0];
        int min_level = quest_row["min_level"].is_null() ? 1 : quest_row["min_level"].as<int>();
        int max_level = quest_row["max_level"].is_null() ? 100 : quest_row["max_level"].as<int>();
        bool hidden = quest_row["hidden"].as<bool>();
        bool repeatable = quest_row["repeatable"].as<bool>();

        // Check level range
        if (level < min_level || level > max_level) {
            return false;
        }

        // Check if hidden
        if (hidden) {
            return false;
        }

        // Check if already in progress
        auto in_progress_result = txn.exec_params(R"(
            SELECT 1 FROM "CharacterQuest"
            WHERE character_id = $1
              AND quest_zone_id = $2
              AND quest_id = $3
              AND status = 'IN_PROGRESS'
        )", std::string(character_id), zone_id, quest_id);

        if (!in_progress_result.empty()) {
            return false; // Already in progress
        }

        // Check if completed and not repeatable
        if (!repeatable) {
            auto completed_result = txn.exec_params(R"(
                SELECT 1 FROM "CharacterQuest"
                WHERE character_id = $1
                  AND quest_zone_id = $2
                  AND quest_id = $3
                  AND status = 'COMPLETED'
            )", std::string(character_id), zone_id, quest_id);

            if (!completed_result.empty()) {
                return false; // Already completed and not repeatable
            }
        }

        // Check prerequisites
        auto prereq_result = txn.exec_params(R"(
            SELECT qp.prerequisite_quest_zone_id, qp.prerequisite_quest_id
            FROM "QuestPrerequisite" qp
            WHERE qp.quest_zone_id = $1 AND qp.quest_id = $2
              AND NOT EXISTS (
                  SELECT 1 FROM "CharacterQuest" cq
                  WHERE cq.character_id = $3
                    AND cq.quest_zone_id = qp.prerequisite_quest_zone_id
                    AND cq.quest_id = qp.prerequisite_quest_id
                    AND cq.status = 'COMPLETED'
              )
        )", zone_id, quest_id, std::string(character_id));

        if (!prereq_result.empty()) {
            return false; // Prerequisites not met
        }

        return true;

    } catch (const std::exception& e) {
        logger->error("Database error checking quest acceptance: {}", e.what());
        return std::unexpected(Errors::DatabaseError(e.what()));
    }
}

// ============================================================================
// Character Quest Progress Queries
// ============================================================================

Result<CharacterQuestProgress> load_character_quest(
    pqxx::work& txn,
    std::string_view character_id,
    int zone_id, int quest_id) {

    auto logger = Log::database();

    try {
        auto result = txn.exec_params(R"(
            SELECT id, status, current_phase_id, accepted_at, completed_at, completion_count
            FROM "CharacterQuest"
            WHERE character_id = $1 AND quest_zone_id = $2 AND quest_id = $3
        )", std::string(character_id), zone_id, quest_id);

        if (result.empty()) {
            return std::unexpected(Errors::NotFound(
                fmt::format("Character quest progress for {}:{}", zone_id, quest_id)));
        }

        const auto& row = result[0];
        CharacterQuestProgress progress;
        progress.character_id = std::string(character_id);
        progress.quest_id = EntityId(zone_id, quest_id);
        progress.status = parse_quest_status(row["status"].as<std::string>());

        if (!row["current_phase_id"].is_null()) {
            progress.current_phase_id = row["current_phase_id"].as<int>();
        }

        // Parse timestamps
        std::string accepted_str = row["accepted_at"].as<std::string>();
        std::tm tm = {};
        std::istringstream ss(accepted_str);
        ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
        progress.accepted_at = std::chrono::system_clock::from_time_t(std::mktime(&tm));

        if (!row["completed_at"].is_null()) {
            std::string completed_str = row["completed_at"].as<std::string>();
            std::tm tm2 = {};
            std::istringstream ss2(completed_str);
            ss2 >> std::get_time(&tm2, "%Y-%m-%d %H:%M:%S");
            progress.completed_at = std::chrono::system_clock::from_time_t(std::mktime(&tm2));
        }

        progress.completion_count = row["completion_count"].as<int>();

        // Load objective progress
        std::string cq_id = row["id"].as<std::string>();
        auto obj_result = txn.exec_params(R"(
            SELECT phase_id, objective_id, current_count, completed, completed_at
            FROM "CharacterQuestObjective"
            WHERE character_quest_id = $1
        )", cq_id);

        for (const auto& obj_row : obj_result) {
            CharacterObjectiveProgress obj_progress;
            obj_progress.phase_id = obj_row["phase_id"].as<int>();
            obj_progress.objective_id = obj_row["objective_id"].as<int>();
            obj_progress.current_count = obj_row["current_count"].as<int>();
            obj_progress.completed = obj_row["completed"].as<bool>();

            if (!obj_row["completed_at"].is_null()) {
                std::string obj_completed_str = obj_row["completed_at"].as<std::string>();
                std::tm tm3 = {};
                std::istringstream ss3(obj_completed_str);
                ss3 >> std::get_time(&tm3, "%Y-%m-%d %H:%M:%S");
                obj_progress.completed_at = std::chrono::system_clock::from_time_t(std::mktime(&tm3));
            }

            progress.objective_progress.push_back(std::move(obj_progress));
        }

        return progress;

    } catch (const std::exception& e) {
        logger->error("Database error loading character quest: {}", e.what());
        return std::unexpected(Errors::DatabaseError(e.what()));
    }
}

Result<std::vector<CharacterQuestProgress>> load_character_active_quests(
    pqxx::work& txn,
    std::string_view character_id) {

    auto logger = Log::database();

    try {
        auto result = txn.exec_params(R"(
            SELECT quest_zone_id, quest_id
            FROM "CharacterQuest"
            WHERE character_id = $1 AND status = 'IN_PROGRESS'
        )", std::string(character_id));

        std::vector<CharacterQuestProgress> quests;
        for (const auto& row : result) {
            auto progress_result = load_character_quest(
                txn, character_id,
                row["quest_zone_id"].as<int>(),
                row["quest_id"].as<int>());
            if (progress_result) {
                quests.push_back(std::move(*progress_result));
            }
        }

        return quests;

    } catch (const std::exception& e) {
        logger->error("Database error loading active quests: {}", e.what());
        return std::unexpected(Errors::DatabaseError(e.what()));
    }
}

Result<std::vector<CharacterQuestProgress>> load_character_completed_quests(
    pqxx::work& txn,
    std::string_view character_id) {

    auto logger = Log::database();

    try {
        auto result = txn.exec_params(R"(
            SELECT quest_zone_id, quest_id
            FROM "CharacterQuest"
            WHERE character_id = $1 AND status = 'COMPLETED'
            ORDER BY completed_at DESC
        )", std::string(character_id));

        std::vector<CharacterQuestProgress> quests;
        for (const auto& row : result) {
            auto progress_result = load_character_quest(
                txn, character_id,
                row["quest_zone_id"].as<int>(),
                row["quest_id"].as<int>());
            if (progress_result) {
                quests.push_back(std::move(*progress_result));
            }
        }

        return quests;

    } catch (const std::exception& e) {
        logger->error("Database error loading completed quests: {}", e.what());
        return std::unexpected(Errors::DatabaseError(e.what()));
    }
}

// ============================================================================
// Quest Progress Mutations
// ============================================================================

Result<void> start_quest(
    pqxx::work& txn,
    std::string_view character_id,
    int zone_id, int quest_id) {

    auto logger = Log::database();
    logger->info("Character {} starting quest {}:{}", character_id, zone_id, quest_id);

    try {
        // Get the first phase ID
        auto phase_result = txn.exec_params(R"(
            SELECT id FROM "QuestPhase"
            WHERE quest_zone_id = $1 AND quest_id = $2
            ORDER BY "order" ASC
            LIMIT 1
        )", zone_id, quest_id);

        int first_phase_id = 0;
        if (!phase_result.empty()) {
            first_phase_id = phase_result[0]["id"].as<int>();
        }

        // Insert character quest record
        auto insert_result = txn.exec_params(R"(
            INSERT INTO "CharacterQuest"
                (id, character_id, quest_zone_id, quest_id, status, current_phase_id, accepted_at, completion_count)
            VALUES
                (gen_random_uuid(), $1, $2, $3, 'IN_PROGRESS', $4, NOW(), 0)
            RETURNING id
        )", std::string(character_id), zone_id, quest_id, first_phase_id);

        std::string cq_id = insert_result[0]["id"].as<std::string>();

        // Initialize objective progress for all objectives
        auto objectives_result = txn.exec_params(R"(
            SELECT phase_id, id FROM "QuestObjective"
            WHERE quest_zone_id = $1 AND quest_id = $2
        )", zone_id, quest_id);

        for (const auto& obj_row : objectives_result) {
            txn.exec_params(R"(
                INSERT INTO "CharacterQuestObjective"
                    (id, character_quest_id, phase_id, objective_id, current_count, completed)
                VALUES
                    (gen_random_uuid(), $1, $2, $3, 0, false)
            )", cq_id, obj_row["phase_id"].as<int>(), obj_row["id"].as<int>());
        }

        logger->info("Character {} started quest {}:{}", character_id, zone_id, quest_id);
        return Success();

    } catch (const std::exception& e) {
        logger->error("Database error starting quest: {}", e.what());
        return std::unexpected(Errors::DatabaseError(e.what()));
    }
}

Result<void> update_objective_progress(
    pqxx::work& txn,
    std::string_view character_id,
    int zone_id, int quest_id,
    int phase_id, int objective_id,
    int new_count) {

    auto logger = Log::database();

    try {
        txn.exec_params(R"(
            UPDATE "CharacterQuestObjective" cqo
            SET current_count = $5
            FROM "CharacterQuest" cq
            WHERE cqo.character_quest_id = cq.id
              AND cq.character_id = $1
              AND cq.quest_zone_id = $2
              AND cq.quest_id = $3
              AND cqo.phase_id = $4
              AND cqo.objective_id = $6
        )", std::string(character_id), zone_id, quest_id, phase_id, new_count, objective_id);

        return Success();

    } catch (const std::exception& e) {
        logger->error("Database error updating objective progress: {}", e.what());
        return std::unexpected(Errors::DatabaseError(e.what()));
    }
}

Result<void> complete_objective(
    pqxx::work& txn,
    std::string_view character_id,
    int zone_id, int quest_id,
    int phase_id, int objective_id) {

    auto logger = Log::database();

    try {
        txn.exec_params(R"(
            UPDATE "CharacterQuestObjective" cqo
            SET completed = true, completed_at = NOW()
            FROM "CharacterQuest" cq
            WHERE cqo.character_quest_id = cq.id
              AND cq.character_id = $1
              AND cq.quest_zone_id = $2
              AND cq.quest_id = $3
              AND cqo.phase_id = $4
              AND cqo.objective_id = $5
        )", std::string(character_id), zone_id, quest_id, phase_id, objective_id);

        return Success();

    } catch (const std::exception& e) {
        logger->error("Database error completing objective: {}", e.what());
        return std::unexpected(Errors::DatabaseError(e.what()));
    }
}

Result<void> advance_phase(
    pqxx::work& txn,
    std::string_view character_id,
    int zone_id, int quest_id,
    int next_phase_id) {

    auto logger = Log::database();

    try {
        txn.exec_params(R"(
            UPDATE "CharacterQuest"
            SET current_phase_id = $4
            WHERE character_id = $1
              AND quest_zone_id = $2
              AND quest_id = $3
              AND status = 'IN_PROGRESS'
        )", std::string(character_id), zone_id, quest_id, next_phase_id);

        return Success();

    } catch (const std::exception& e) {
        logger->error("Database error advancing phase: {}", e.what());
        return std::unexpected(Errors::DatabaseError(e.what()));
    }
}

Result<void> complete_quest(
    pqxx::work& txn,
    std::string_view character_id,
    int zone_id, int quest_id) {

    auto logger = Log::database();
    logger->info("Character {} completing quest {}:{}", character_id, zone_id, quest_id);

    try {
        txn.exec_params(R"(
            UPDATE "CharacterQuest"
            SET status = 'COMPLETED',
                completed_at = NOW(),
                completion_count = completion_count + 1
            WHERE character_id = $1
              AND quest_zone_id = $2
              AND quest_id = $3
              AND status = 'IN_PROGRESS'
        )", std::string(character_id), zone_id, quest_id);

        return Success();

    } catch (const std::exception& e) {
        logger->error("Database error completing quest: {}", e.what());
        return std::unexpected(Errors::DatabaseError(e.what()));
    }
}

Result<void> abandon_quest(
    pqxx::work& txn,
    std::string_view character_id,
    int zone_id, int quest_id) {

    auto logger = Log::database();
    logger->info("Character {} abandoning quest {}:{}", character_id, zone_id, quest_id);

    try {
        txn.exec_params(R"(
            UPDATE "CharacterQuest"
            SET status = 'ABANDONED'
            WHERE character_id = $1
              AND quest_zone_id = $2
              AND quest_id = $3
              AND status = 'IN_PROGRESS'
        )", std::string(character_id), zone_id, quest_id);

        return Success();

    } catch (const std::exception& e) {
        logger->error("Database error abandoning quest: {}", e.what());
        return std::unexpected(Errors::DatabaseError(e.what()));
    }
}

// ============================================================================
// Quest Variables
// ============================================================================

Result<void> set_quest_variable(
    pqxx::work& txn,
    std::string_view character_id,
    int zone_id, int quest_id,
    const std::string& name,
    const nlohmann::json& value) {

    auto logger = Log::database();
    logger->debug("Setting quest variable '{}' for character {} quest {}:{}",
                  name, character_id, zone_id, quest_id);

    try {
        // Get current variables JSON
        auto result = txn.exec_params(R"(
            SELECT variables FROM "CharacterQuest"
            WHERE character_id = $1
              AND quest_zone_id = $2
              AND quest_id = $3
        )", std::string(character_id), zone_id, quest_id);

        if (result.empty()) {
            return std::unexpected(Errors::NotFound(
                fmt::format("Character quest {}:{}", zone_id, quest_id)));
        }

        // Parse existing variables or create empty object
        nlohmann::json variables;
        std::string vars_str = result[0]["variables"].as<std::string>();
        if (!vars_str.empty() && vars_str != "{}") {
            try {
                variables = nlohmann::json::parse(vars_str);
            } catch (const nlohmann::json::exception&) {
                variables = nlohmann::json::object();
            }
        } else {
            variables = nlohmann::json::object();
        }

        // Set the variable
        variables[name] = value;

        // Update database
        txn.exec_params(R"(
            UPDATE "CharacterQuest"
            SET variables = $4::jsonb
            WHERE character_id = $1
              AND quest_zone_id = $2
              AND quest_id = $3
        )", std::string(character_id), zone_id, quest_id, variables.dump());

        logger->debug("Set quest variable '{}' = {}", name, value.dump());
        return Success();

    } catch (const std::exception& e) {
        logger->error("Database error setting quest variable: {}", e.what());
        return std::unexpected(Errors::DatabaseError(e.what()));
    }
}

Result<nlohmann::json> get_quest_variable(
    pqxx::work& txn,
    std::string_view character_id,
    int zone_id, int quest_id,
    const std::string& name) {

    auto logger = Log::database();
    logger->debug("Getting quest variable '{}' for character {} quest {}:{}",
                  name, character_id, zone_id, quest_id);

    try {
        auto result = txn.exec_params(R"(
            SELECT variables FROM "CharacterQuest"
            WHERE character_id = $1
              AND quest_zone_id = $2
              AND quest_id = $3
        )", std::string(character_id), zone_id, quest_id);

        if (result.empty()) {
            return std::unexpected(Errors::NotFound(
                fmt::format("Character quest {}:{}", zone_id, quest_id)));
        }

        // Parse variables JSON
        std::string vars_str = result[0]["variables"].as<std::string>();
        if (vars_str.empty() || vars_str == "{}") {
            return std::unexpected(Errors::NotFound(
                fmt::format("Quest variable '{}'", name)));
        }

        nlohmann::json variables;
        try {
            variables = nlohmann::json::parse(vars_str);
        } catch (const nlohmann::json::exception&) {
            return std::unexpected(Errors::NotFound(
                fmt::format("Quest variable '{}'", name)));
        }

        // Get the variable
        if (!variables.contains(name)) {
            return std::unexpected(Errors::NotFound(
                fmt::format("Quest variable '{}'", name)));
        }

        return variables[name];

    } catch (const std::exception& e) {
        logger->error("Database error getting quest variable: {}", e.what());
        return std::unexpected(Errors::DatabaseError(e.what()));
    }
}

// ============================================================================
// Trigger-Based Quest Queries
// ============================================================================

Result<std::vector<QuestData>> load_level_triggered_quests(pqxx::work& txn, int level) {
    auto logger = Log::database();
    logger->debug("Loading quests triggered at level {}", level);

    try {
        auto result = txn.exec_params(R"(
            SELECT zone_id, id FROM "Quest"
            WHERE trigger_type = 'LEVEL'
              AND trigger_level = $1
              AND hidden = false
            ORDER BY zone_id, id
        )", level);

        std::vector<QuestData> quests;
        for (const auto& row : result) {
            auto quest_result = load_quest(txn, row["zone_id"].as<int>(), row["id"].as<int>());
            if (quest_result) {
                quests.push_back(std::move(*quest_result));
            }
        }

        logger->debug("Found {} level-triggered quests at level {}", quests.size(), level);
        return quests;

    } catch (const std::exception& e) {
        logger->error("Database error loading level-triggered quests: {}", e.what());
        return std::unexpected(Errors::DatabaseError(e.what()));
    }
}

Result<std::vector<QuestData>> load_item_triggered_quests(
    pqxx::work& txn,
    int item_zone_id, int item_id) {

    auto logger = Log::database();
    logger->debug("Loading quests triggered by item {}:{}", item_zone_id, item_id);

    try {
        auto result = txn.exec_params(R"(
            SELECT zone_id, id FROM "Quest"
            WHERE trigger_type = 'ITEM'
              AND trigger_item_zone_id = $1
              AND trigger_item_id = $2
              AND hidden = false
            ORDER BY zone_id, id
        )", item_zone_id, item_id);

        std::vector<QuestData> quests;
        for (const auto& row : result) {
            auto quest_result = load_quest(txn, row["zone_id"].as<int>(), row["id"].as<int>());
            if (quest_result) {
                quests.push_back(std::move(*quest_result));
            }
        }

        logger->debug("Found {} item-triggered quests for item {}:{}", quests.size(), item_zone_id, item_id);
        return quests;

    } catch (const std::exception& e) {
        logger->error("Database error loading item-triggered quests: {}", e.what());
        return std::unexpected(Errors::DatabaseError(e.what()));
    }
}

Result<std::vector<QuestData>> load_room_triggered_quests(
    pqxx::work& txn,
    int room_zone_id, int room_id) {

    auto logger = Log::database();
    logger->debug("Loading quests triggered by room {}:{}", room_zone_id, room_id);

    try {
        auto result = txn.exec_params(R"(
            SELECT zone_id, id FROM "Quest"
            WHERE trigger_type = 'ROOM'
              AND trigger_room_zone_id = $1
              AND trigger_room_id = $2
              AND hidden = false
            ORDER BY zone_id, id
        )", room_zone_id, room_id);

        std::vector<QuestData> quests;
        for (const auto& row : result) {
            auto quest_result = load_quest(txn, row["zone_id"].as<int>(), row["id"].as<int>());
            if (quest_result) {
                quests.push_back(std::move(*quest_result));
            }
        }

        logger->debug("Found {} room-triggered quests for room {}:{}", quests.size(), room_zone_id, room_id);
        return quests;

    } catch (const std::exception& e) {
        logger->error("Database error loading room-triggered quests: {}", e.what());
        return std::unexpected(Errors::DatabaseError(e.what()));
    }
}

Result<std::vector<QuestData>> load_skill_triggered_quests(pqxx::work& txn, int ability_id) {
    auto logger = Log::database();
    logger->debug("Loading quests triggered by ability {}", ability_id);

    try {
        auto result = txn.exec_params(R"(
            SELECT zone_id, id FROM "Quest"
            WHERE trigger_type = 'SKILL'
              AND trigger_ability_id = $1
              AND hidden = false
            ORDER BY zone_id, id
        )", ability_id);

        std::vector<QuestData> quests;
        for (const auto& row : result) {
            auto quest_result = load_quest(txn, row["zone_id"].as<int>(), row["id"].as<int>());
            if (quest_result) {
                quests.push_back(std::move(*quest_result));
            }
        }

        logger->debug("Found {} skill-triggered quests for ability {}", quests.size(), ability_id);
        return quests;

    } catch (const std::exception& e) {
        logger->error("Database error loading skill-triggered quests: {}", e.what());
        return std::unexpected(Errors::DatabaseError(e.what()));
    }
}

Result<std::vector<QuestData>> load_event_triggered_quests(pqxx::work& txn, int event_id) {
    auto logger = Log::database();
    logger->debug("Loading quests triggered by event {}", event_id);

    try {
        auto result = txn.exec_params(R"(
            SELECT zone_id, id FROM "Quest"
            WHERE trigger_type = 'EVENT'
              AND trigger_event_id = $1
              AND hidden = false
            ORDER BY zone_id, id
        )", event_id);

        std::vector<QuestData> quests;
        for (const auto& row : result) {
            auto quest_result = load_quest(txn, row["zone_id"].as<int>(), row["id"].as<int>());
            if (quest_result) {
                quests.push_back(std::move(*quest_result));
            }
        }

        logger->debug("Found {} event-triggered quests for event {}", quests.size(), event_id);
        return quests;

    } catch (const std::exception& e) {
        logger->error("Database error loading event-triggered quests: {}", e.what());
        return std::unexpected(Errors::DatabaseError(e.what()));
    }
}

Result<std::vector<QuestData>> load_auto_triggered_quests(pqxx::work& txn) {
    auto logger = Log::database();
    logger->debug("Loading auto-triggered quests");

    try {
        auto result = txn.exec_params(R"(
            SELECT zone_id, id FROM "Quest"
            WHERE trigger_type = 'AUTO'
              AND hidden = false
            ORDER BY zone_id, id
        )");

        std::vector<QuestData> quests;
        for (const auto& row : result) {
            auto quest_result = load_quest(txn, row["zone_id"].as<int>(), row["id"].as<int>());
            if (quest_result) {
                quests.push_back(std::move(*quest_result));
            }
        }

        logger->debug("Found {} auto-triggered quests", quests.size());
        return quests;

    } catch (const std::exception& e) {
        logger->error("Database error loading auto-triggered quests: {}", e.what());
        return std::unexpected(Errors::DatabaseError(e.what()));
    }
}

} // namespace QuestQueries
