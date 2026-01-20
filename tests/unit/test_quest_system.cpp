#include "../../src/quests/quest_manager.hpp"
#include "../../src/database/quest_queries.hpp"
#include "../../src/core/ids.hpp"

#include <catch2/catch_test_macros.hpp>

// Note: Using explicit namespace prefixes to avoid ambiguity between
// FieryMUD::QuestStatus and QuestQueries::QuestStatus

/**
 * Quest System Unit Tests
 * Tests core quest data structures and enums
 */
TEST_CASE("Quest: FieryMUD::QuestStatus enum", "[quest][unit]") {
    SECTION("QuestStatus values are distinct") {
        using FieryMUD::QuestStatus;
        REQUIRE(static_cast<int>(QuestStatus::None) != static_cast<int>(QuestStatus::Available));
        REQUIRE(static_cast<int>(QuestStatus::Available) != static_cast<int>(QuestStatus::InProgress));
        REQUIRE(static_cast<int>(QuestStatus::InProgress) != static_cast<int>(QuestStatus::Completed));
        REQUIRE(static_cast<int>(QuestStatus::Completed) != static_cast<int>(QuestStatus::Failed));
        REQUIRE(static_cast<int>(QuestStatus::Failed) != static_cast<int>(QuestStatus::Abandoned));
    }
}

TEST_CASE("Quest: QuestQueries::QuestStatus enum", "[quest][unit]") {
    SECTION("Quest status values are usable") {
        REQUIRE(static_cast<int>(QuestQueries::QuestStatus::IN_PROGRESS) !=
                static_cast<int>(QuestQueries::QuestStatus::COMPLETED));
        REQUIRE(static_cast<int>(QuestQueries::QuestStatus::COMPLETED) !=
                static_cast<int>(QuestQueries::QuestStatus::FAILED));
    }
}

TEST_CASE("Quest: QuestObjectiveType enum", "[quest][unit]") {
    SECTION("Objective types are distinct") {
        REQUIRE(static_cast<int>(QuestQueries::QuestObjectiveType::KILL_MOB) !=
                static_cast<int>(QuestQueries::QuestObjectiveType::COLLECT_ITEM));
        REQUIRE(static_cast<int>(QuestQueries::QuestObjectiveType::COLLECT_ITEM) !=
                static_cast<int>(QuestQueries::QuestObjectiveType::DELIVER_ITEM));
        REQUIRE(static_cast<int>(QuestQueries::QuestObjectiveType::DELIVER_ITEM) !=
                static_cast<int>(QuestQueries::QuestObjectiveType::TALK_TO_NPC));
    }
}

TEST_CASE("Quest: QuestData structure", "[quest][unit]") {
    SECTION("QuestData can be default constructed") {
        QuestQueries::QuestData quest{};
        REQUIRE_FALSE(quest.id.is_valid());
        REQUIRE(quest.name.empty());
        REQUIRE(quest.min_level == 0);
        REQUIRE(quest.max_level == 0);
        REQUIRE_FALSE(quest.repeatable);
    }

    SECTION("QuestData fields can be assigned") {
        QuestQueries::QuestData quest{};
        quest.id = EntityId{30, 1};
        quest.name = "Test Quest";
        quest.min_level = 1;
        quest.max_level = 50;
        quest.repeatable = true;

        REQUIRE(quest.id.zone_id() == 30);
        REQUIRE(quest.id.local_id() == 1);
        REQUIRE(quest.name == "Test Quest");
        REQUIRE(quest.min_level == 1);
        REQUIRE(quest.max_level == 50);
        REQUIRE(quest.repeatable);
    }
}

TEST_CASE("Quest: QuestObjective structure", "[quest][unit]") {
    SECTION("QuestObjective can be default constructed") {
        QuestQueries::QuestObjective objective{};
        REQUIRE(objective.id == 0);
        REQUIRE(objective.phase_id == 0);
        REQUIRE(objective.required_count == 0);
    }

    SECTION("Kill mob objective setup") {
        QuestQueries::QuestObjective objective{};
        objective.id = 1;
        objective.phase_id = 1;
        objective.type = QuestQueries::QuestObjectiveType::KILL_MOB;
        objective.player_description = "Kill 10 rats";
        objective.required_count = 10;
        objective.target_mob = EntityId{30, 5};

        REQUIRE(objective.id == 1);
        REQUIRE(objective.type == QuestQueries::QuestObjectiveType::KILL_MOB);
        REQUIRE(objective.required_count == 10);
        REQUIRE(objective.target_mob.has_value());
        REQUIRE(objective.target_mob->zone_id() == 30);
        REQUIRE(objective.target_mob->local_id() == 5);
    }
}

TEST_CASE("Quest: QuestPhase structure", "[quest][unit]") {
    SECTION("Phase with multiple objectives") {
        QuestQueries::QuestPhase phase{};
        phase.id = 1;
        phase.name = "Phase 1: Investigation";
        phase.order = 0;

        // Add objectives
        QuestQueries::QuestObjective obj1{};
        obj1.id = 1;
        obj1.phase_id = 1;
        obj1.type = QuestQueries::QuestObjectiveType::TALK_TO_NPC;
        obj1.player_description = "Talk to the innkeeper";
        obj1.required_count = 1;

        QuestQueries::QuestObjective obj2{};
        obj2.id = 2;
        obj2.phase_id = 1;
        obj2.type = QuestQueries::QuestObjectiveType::COLLECT_ITEM;
        obj2.player_description = "Find the lost key";
        obj2.required_count = 1;

        phase.objectives.push_back(obj1);
        phase.objectives.push_back(obj2);

        REQUIRE(phase.objectives.size() == 2);
        REQUIRE(phase.objectives[0].type == QuestQueries::QuestObjectiveType::TALK_TO_NPC);
        REQUIRE(phase.objectives[1].type == QuestQueries::QuestObjectiveType::COLLECT_ITEM);
    }
}

TEST_CASE("Quest: CharacterObjectiveProgress structure", "[quest][unit]") {
    SECTION("Objective progress tracking") {
        QuestQueries::CharacterObjectiveProgress progress{};
        progress.objective_id = 1;
        progress.phase_id = 1;
        progress.current_count = 5;
        progress.completed = false;

        REQUIRE(progress.current_count == 5);
        REQUIRE_FALSE(progress.completed);

        // Simulate progress completion
        progress.current_count = 10;
        progress.completed = true;

        REQUIRE(progress.current_count == 10);
        REQUIRE(progress.completed);
    }
}

TEST_CASE("Quest: QuestReward structure", "[quest][unit]") {
    SECTION("Experience reward") {
        QuestQueries::QuestReward reward{};
        reward.type = QuestQueries::QuestRewardType::EXPERIENCE;
        reward.amount = 1000;

        REQUIRE(reward.type == QuestQueries::QuestRewardType::EXPERIENCE);
        REQUIRE(reward.amount.has_value());
        REQUIRE(*reward.amount == 1000);
    }

    SECTION("Item reward") {
        QuestQueries::QuestReward reward{};
        reward.type = QuestQueries::QuestRewardType::ITEM;
        reward.object = EntityId{30, 15};

        REQUIRE(reward.type == QuestQueries::QuestRewardType::ITEM);
        REQUIRE(reward.object.has_value());
        REQUIRE(reward.object->zone_id() == 30);
        REQUIRE(reward.object->local_id() == 15);
    }
}

TEST_CASE("Quest: QuestManager singleton", "[quest][unit]") {
    SECTION("Singleton instance is consistent") {
        auto& manager1 = FieryMUD::QuestManager::instance();
        auto& manager2 = FieryMUD::QuestManager::instance();

        // Both references should point to the same instance
        REQUIRE(&manager1 == &manager2);
    }

    SECTION("Manager exists") {
        auto& manager = FieryMUD::QuestManager::instance();
        // Singleton should exist
        REQUIRE(&manager != nullptr);
    }
}

TEST_CASE("Quest: QuestTriggerType enum", "[quest][unit]") {
    SECTION("Trigger types are distinct") {
        REQUIRE(static_cast<int>(QuestQueries::QuestTriggerType::MOB) !=
                static_cast<int>(QuestQueries::QuestTriggerType::LEVEL));
        REQUIRE(static_cast<int>(QuestQueries::QuestTriggerType::LEVEL) !=
                static_cast<int>(QuestQueries::QuestTriggerType::ITEM));
        REQUIRE(static_cast<int>(QuestQueries::QuestTriggerType::ITEM) !=
                static_cast<int>(QuestQueries::QuestTriggerType::ROOM));
        REQUIRE(static_cast<int>(QuestQueries::QuestTriggerType::ROOM) !=
                static_cast<int>(QuestQueries::QuestTriggerType::SKILL));
        REQUIRE(static_cast<int>(QuestQueries::QuestTriggerType::SKILL) !=
                static_cast<int>(QuestQueries::QuestTriggerType::EVENT));
    }
}

// ============================================================================
// QuestManager Functionality Tests
// ============================================================================

TEST_CASE("Quest: QuestManager initialization", "[quest][unit]") {
    auto& manager = FieryMUD::QuestManager::instance();

    SECTION("Manager can initialize successfully") {
        // Initialize should work (or return true if already initialized)
        bool result = manager.initialize();
        REQUIRE(result == true);
    }

    SECTION("Manager tracks initialization state") {
        // After initialize, is_initialized should return true
        (void)manager.initialize();
        REQUIRE(manager.is_initialized() == true);
    }

    SECTION("Manager can be shutdown and reinitialized") {
        (void)manager.initialize();
        manager.shutdown();

        // After shutdown, reinitialize should work
        bool result = manager.initialize();
        REQUIRE(result == true);
    }
}

TEST_CASE("Quest: QuestManager statistics", "[quest][unit]") {
    auto& manager = FieryMUD::QuestManager::instance();
    (void)manager.initialize();

    SECTION("Stats can be reset") {
        manager.reset_stats();
        const auto& stats = manager.stats();

        REQUIRE(stats.quests_started == 0);
        REQUIRE(stats.quests_completed == 0);
        REQUIRE(stats.quests_abandoned == 0);
        REQUIRE(stats.objectives_completed == 0);
        REQUIRE(stats.trigger_activations == 0);
    }

    SECTION("Stats structure has all expected fields") {
        const auto& stats = manager.stats();

        // Just verify these fields exist and are accessible
        [[maybe_unused]] auto started = stats.quests_started;
        [[maybe_unused]] auto completed = stats.quests_completed;
        [[maybe_unused]] auto abandoned = stats.quests_abandoned;
        [[maybe_unused]] auto objectives = stats.objectives_completed;
        [[maybe_unused]] auto triggers = stats.trigger_activations;

        REQUIRE(true); // If we got here, all fields exist
    }
}

TEST_CASE("Quest: QuestManager quest count", "[quest][unit]") {
    auto& manager = FieryMUD::QuestManager::instance();
    (void)manager.initialize();

    SECTION("Empty manager has zero quest count") {
        manager.clear_all_quests();
        REQUIRE(manager.quest_count() == 0);
    }

    SECTION("Quest count for non-existent zone is zero") {
        manager.clear_all_quests();
        REQUIRE(manager.quest_count(999) == 0);
    }
}

TEST_CASE("Quest: QuestManager quest lookup", "[quest][unit]") {
    auto& manager = FieryMUD::QuestManager::instance();
    (void)manager.initialize();

    SECTION("Looking up non-existent quest returns nullptr") {
        auto* quest = manager.get_quest(999, 999);
        REQUIRE(quest == nullptr);
    }

    SECTION("Looking up by invalid EntityId returns nullptr") {
        EntityId invalid_id{999, 999};
        auto* quest = manager.get_quest(invalid_id);
        REQUIRE(quest == nullptr);
    }

    SECTION("Get zone quests for non-existent zone returns empty vector") {
        auto quests = manager.get_zone_quests(999);
        REQUIRE(quests.empty());
    }
}

TEST_CASE("Quest: QuestManager available quests", "[quest][unit]") {
    auto& manager = FieryMUD::QuestManager::instance();
    (void)manager.initialize();
    manager.clear_all_quests();

    SECTION("Available quests for non-existent character is empty when no quests loaded") {
        auto quests = manager.get_available_quests("nonexistent-char-id", 10);
        REQUIRE(quests.empty());
    }
}

TEST_CASE("Quest: QuestManager zone operations", "[quest][unit]") {
    auto& manager = FieryMUD::QuestManager::instance();
    (void)manager.initialize();

    SECTION("Clear zone quests works on non-existent zone") {
        // Should not crash or error
        manager.clear_zone_quests(999);
        REQUIRE(manager.quest_count(999) == 0);
    }

    SECTION("Clear all quests resets state") {
        manager.clear_all_quests();
        REQUIRE(manager.quest_count() == 0);
    }
}

TEST_CASE("Quest: QuestManager without database", "[quest][unit]") {
    auto& manager = FieryMUD::QuestManager::instance();
    (void)manager.initialize();

    SECTION("Load zone quests returns zero without database") {
        // Without a database connection, load should return 0 (not error)
        auto result = manager.load_zone_quests(30);
        REQUIRE(result.has_value());
        REQUIRE(*result == 0);
    }

    SECTION("has_quest_in_progress returns false without database") {
        bool result = manager.has_quest_in_progress("test-char", 30, 1);
        REQUIRE(result == false);
    }

    SECTION("has_completed_quest returns false without database") {
        bool result = manager.has_completed_quest("test-char", 30, 1);
        REQUIRE(result == false);
    }
}

TEST_CASE("Quest: Full quest data structure assembly", "[quest][unit]") {
    SECTION("Create a complete quest with all components") {
        QuestQueries::QuestData quest{};
        quest.id = EntityId{30, 1};
        quest.name = "The Lost Artifact";
        quest.description = "Find the ancient artifact hidden in the dungeon.";
        quest.min_level = 10;
        quest.max_level = 20;
        quest.repeatable = false;
        quest.hidden = false;
        quest.trigger_type = QuestQueries::QuestTriggerType::MOB;
        quest.giver_mob = EntityId{30, 50}; // Quest giver mob

        // Add phase 1
        QuestQueries::QuestPhase phase1{};
        phase1.id = 1;
        phase1.name = "Find the Map";
        phase1.order = 0;
        phase1.description = "Locate the old map in the library.";

        QuestQueries::QuestObjective obj1{};
        obj1.id = 1;
        obj1.phase_id = 1;
        obj1.type = QuestQueries::QuestObjectiveType::COLLECT_ITEM;
        obj1.player_description = "Find the ancient map";
        obj1.required_count = 1;
        obj1.target_object = EntityId{30, 100};
        phase1.objectives.push_back(obj1);

        // Add phase 2
        QuestQueries::QuestPhase phase2{};
        phase2.id = 2;
        phase2.name = "Retrieve the Artifact";
        phase2.order = 1;
        phase2.description = "Use the map to find the artifact.";

        QuestQueries::QuestObjective obj2{};
        obj2.id = 2;
        obj2.phase_id = 2;
        obj2.type = QuestQueries::QuestObjectiveType::KILL_MOB;
        obj2.player_description = "Defeat the guardian";
        obj2.required_count = 1;
        obj2.target_mob = EntityId{30, 60};
        phase2.objectives.push_back(obj2);

        QuestQueries::QuestObjective obj3{};
        obj3.id = 3;
        obj3.phase_id = 2;
        obj3.type = QuestQueries::QuestObjectiveType::COLLECT_ITEM;
        obj3.player_description = "Take the artifact";
        obj3.required_count = 1;
        obj3.target_object = EntityId{30, 101};
        phase2.objectives.push_back(obj3);

        quest.phases.push_back(phase1);
        quest.phases.push_back(phase2);

        // Add rewards
        QuestQueries::QuestReward xp_reward{};
        xp_reward.type = QuestQueries::QuestRewardType::EXPERIENCE;
        xp_reward.amount = 5000;
        quest.rewards.push_back(xp_reward);

        QuestQueries::QuestReward gold_reward{};
        gold_reward.type = QuestQueries::QuestRewardType::GOLD;
        gold_reward.amount = 500;
        quest.rewards.push_back(gold_reward);

        QuestQueries::QuestReward item_reward{};
        item_reward.type = QuestQueries::QuestRewardType::ITEM;
        item_reward.object = EntityId{30, 200};
        quest.rewards.push_back(item_reward);

        // Verify structure
        REQUIRE(quest.id.zone_id() == 30);
        REQUIRE(quest.id.local_id() == 1);
        REQUIRE(quest.name == "The Lost Artifact");
        REQUIRE(quest.phases.size() == 2);
        REQUIRE(quest.phases[0].objectives.size() == 1);
        REQUIRE(quest.phases[1].objectives.size() == 2);
        REQUIRE(quest.rewards.size() == 3);

        // Verify phase order
        REQUIRE(quest.phases[0].order == 0);
        REQUIRE(quest.phases[1].order == 1);

        // Verify objective types
        REQUIRE(quest.phases[0].objectives[0].type == QuestQueries::QuestObjectiveType::COLLECT_ITEM);
        REQUIRE(quest.phases[1].objectives[0].type == QuestQueries::QuestObjectiveType::KILL_MOB);
        REQUIRE(quest.phases[1].objectives[1].type == QuestQueries::QuestObjectiveType::COLLECT_ITEM);

        // Verify reward types
        REQUIRE(quest.rewards[0].type == QuestQueries::QuestRewardType::EXPERIENCE);
        REQUIRE(quest.rewards[1].type == QuestQueries::QuestRewardType::GOLD);
        REQUIRE(quest.rewards[2].type == QuestQueries::QuestRewardType::ITEM);
    }
}

TEST_CASE("Quest: Character quest progress tracking", "[quest][unit]") {
    SECTION("CharacterQuestProgress can track active quest") {
        QuestQueries::CharacterQuestProgress progress{};
        progress.quest_id = EntityId{30, 1};
        progress.status = QuestQueries::QuestStatus::IN_PROGRESS;
        progress.current_phase_id = 1;
        progress.accepted_at = std::chrono::system_clock::now();

        // Add objective progress
        QuestQueries::CharacterObjectiveProgress obj_prog{};
        obj_prog.objective_id = 1;
        obj_prog.phase_id = 1;
        obj_prog.current_count = 5;
        obj_prog.completed = false;
        progress.objective_progress.push_back(obj_prog);

        REQUIRE(progress.quest_id.zone_id() == 30);
        REQUIRE(progress.quest_id.local_id() == 1);
        REQUIRE(progress.status == QuestQueries::QuestStatus::IN_PROGRESS);
        REQUIRE(progress.current_phase_id.has_value());
        REQUIRE(*progress.current_phase_id == 1);
        REQUIRE(progress.objective_progress.size() == 1);
        REQUIRE(progress.objective_progress[0].current_count == 5);
        REQUIRE_FALSE(progress.objective_progress[0].completed);
    }

    SECTION("CharacterQuestProgress can track completed quest") {
        QuestQueries::CharacterQuestProgress progress{};
        progress.quest_id = EntityId{30, 1};
        progress.status = QuestQueries::QuestStatus::COMPLETED;
        progress.accepted_at = std::chrono::system_clock::now();
        progress.completed_at = std::chrono::system_clock::now();
        progress.completion_count = 1;

        REQUIRE(progress.status == QuestQueries::QuestStatus::COMPLETED);
        REQUIRE(progress.completed_at.has_value());
        REQUIRE(progress.completion_count == 1);
    }
}
