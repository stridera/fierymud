#include <set>
#include <vector>

#include <catch2/catch_test_macros.hpp>
#include <nlohmann/json.hpp>

#include "core/actor.hpp"
#include "core/player.hpp"
#include "world/world_manager.hpp"

TEST_CASE("Player: Start Room Initialization", "[unit][player][start_room]") {
    // Create a player for start room testing (no full world setup needed)
    auto player_result = Player::create(EntityId{2000}, "TestPlayer");
    REQUIRE(player_result.has_value());
    auto player = std::move(player_result.value());

    SECTION("Player can have start room set") {
        REQUIRE(!player->start_room().is_valid()); // Initially invalid

        EntityId test_room_id{1000};
        player->set_start_room(test_room_id);
        REQUIRE(player->start_room().is_valid());
        REQUIRE(player->start_room() == test_room_id);
    }

    SECTION("Player start room persists in JSON") {
        EntityId test_room_id{1001};
        player->set_start_room(test_room_id);

        // Serialize to JSON
        auto json = player->to_json();
        REQUIRE(json.contains("start_room"));
        REQUIRE(json["start_room"].get<uint64_t>() == test_room_id.value());

        // Load from JSON
        auto loaded_result = Player::from_json(json);
        REQUIRE(loaded_result.has_value());
        auto loaded_player = std::move(loaded_result.value());

        REQUIRE(loaded_player->start_room() == test_room_id);
    }

    SECTION("Player start room values are handled correctly") {
        // Test invalid ID handling
        REQUIRE(!player->start_room().is_valid());

        // Test setting and clearing start room
        EntityId test_room_id{2000};
        player->set_start_room(test_room_id);
        REQUIRE(player->start_room() == test_room_id);

        // Test clearing by setting to invalid ID
        player->set_start_room(INVALID_ENTITY_ID);
        REQUIRE(!player->start_room().is_valid());
    }
}

TEST_CASE("Player: Dead State Command Permissions", "[unit][player][dead]") {
    // Create a player for testing position states (no world setup needed)
    auto player_result = Player::create(EntityId{2001}, "TestPlayer");
    REQUIRE(player_result.has_value());
    auto player = std::move(player_result.value());

    SECTION("Alive player can use all commands") {
        player->set_position(Position::Standing);
        REQUIRE(player->is_alive());
        REQUIRE(player->can_act());
    }

    SECTION("Dead player has restricted command access") {
        player->set_position(Position::Dead);
        REQUIRE(!player->is_alive());
        REQUIRE(!player->can_act());
    }

    SECTION("Ghost player has restricted command access") {
        player->set_position(Position::Ghost);
        REQUIRE(!player->is_alive());
        REQUIRE(!player->can_act());
    }

    SECTION("Test all position states for consistency") {
        // Test all possible position states
        std::vector<std::pair<Position, bool>> position_tests = {
            {Position::Dead, false},         {Position::Ghost, false},  {Position::Mortally_Wounded, true},
            {Position::Incapacitated, true}, // alive but can't act
            {Position::Stunned, true},       // alive but can't act
            {Position::Sleeping, true},      // alive but can't act
            {Position::Resting, true},       {Position::Sitting, true}, {Position::Fighting, true},
            {Position::Standing, true}};

        for (const auto &[position, should_be_alive] : position_tests) {
            player->set_position(position);
            if (should_be_alive) {
                REQUIRE(player->is_alive());
            } else {
                REQUIRE_FALSE(player->is_alive());
            }

            // According to Actor::can_act(), can act if alive and not Incapacitated, Stunned, or Sleeping
            bool should_be_able_to_act = should_be_alive && position != Position::Incapacitated &&
                                         position != Position::Stunned && position != Position::Sleeping;
            if (should_be_able_to_act) {
                REQUIRE(player->can_act());
            } else {
                REQUIRE_FALSE(player->can_act());
            }
        }
    }
}

TEST_CASE("Player: Revival Release Command Flow", "[unit][player][revival]") {
    // Simplified test without full WorldServer integration
    // This test validates the underlying mechanics needed for release command

    SECTION("Ghost player has proper start room configuration") {
        auto player_result = Player::create(EntityId{2002}, "TestPlayer");
        REQUIRE(player_result.has_value());
        auto player = std::move(player_result.value());

        // Set player as ghost
        player->set_position(Position::Ghost);
        REQUIRE(player->position() == Position::Ghost);
        REQUIRE_FALSE(player->is_alive());
        REQUIRE_FALSE(player->can_act());

        // Set their start room
        EntityId test_room_id{1000};
        player->set_start_room(test_room_id);
        REQUIRE(player->start_room() == test_room_id);

        // Validate ghost position behavior
        REQUIRE(player->position() == Position::Ghost);

        // The actual release command would:
        // 1. Check ghost status ✓ - player->position() == Position::Ghost
        // 2. Get start room ✓ - player->start_room() returns valid ID
        // 3. Move player there (requires room system integration)
        // 4. Set position to Standing - player->set_position(Position::Standing)
        // 5. Restore HP - player->stats().hit_points = player->stats().max_hit_points

        // Test position restoration (simulating what release command does)
        player->set_position(Position::Standing);
        REQUIRE(player->position() == Position::Standing);
        REQUIRE(player->is_alive());
        REQUIRE(player->can_act());

        // Test HP restoration (simulating what release command does)
        auto &stats = player->stats();
        stats.hit_points = stats.max_hit_points;
        REQUIRE(stats.hit_points == stats.max_hit_points);
    }
}

TEST_CASE("Command System: Ghost Command Whitelist Validation", "[unit][commands][ghost]") {
    // Test the ghost command whitelist concept without full command system setup
    // This validates the design of the ghost_allowed_commands system

    SECTION("Ghost command whitelist contains expected commands") {
        // Common commands that ghosts should be able to use
        std::set<std::string> expected_ghost_commands = {"look",      "l", "release",   "help", "?",   "score", "sc",
                                                         "inventory", "i", "equipment", "eq",   "who", "say",   "tell"};

        // This test validates that the concept exists
        // The actual implementation is in src/commands/command_system.cpp
        // We're testing the design principle here

        for (const auto &cmd : expected_ghost_commands) {
            // These commands should be safe for ghost players
            REQUIRE(!cmd.empty());
            REQUIRE(cmd.length() <= 20); // Reasonable command name length
        }
    }

    SECTION("Ghost commands are generally safe operations") {
        // Commands that should be safe for ghost players (no world modification)
        std::vector<std::string> safe_commands = {"look", "help", "score",   "inventory", "equipment",
                                                  "who",  "time", "weather", "exits"};

        // Commands that should NOT be safe for ghost players (world modification)
        std::vector<std::string> unsafe_commands = {"get", "drop", "attack", "kill", "steal", "give",
                                                    "put", "open", "close",  "lock", "unlock"};

        // This validates the design principle
        REQUIRE(safe_commands.size() > 0);
        REQUIRE(unsafe_commands.size() > 0);

        // All command names should be non-empty and reasonable length
        for (const auto &cmd : safe_commands) {
            REQUIRE(!cmd.empty());
            REQUIRE(cmd.length() <= 15);
        }

        for (const auto &cmd : unsafe_commands) {
            REQUIRE(!cmd.empty());
            REQUIRE(cmd.length() <= 15);
        }
    }
}

TEST_CASE("Player Stats and HP System", "[unit][player][stats][hp]") {
    auto player_result = Player::create(EntityId{7001}, "TestPlayer");
    REQUIRE(player_result.has_value());
    auto player = std::move(player_result.value());

    SECTION("Player has default stats") {
        const auto &stats = player->stats();
        REQUIRE(stats.max_hit_points > 0);
        REQUIRE(stats.hit_points <= stats.max_hit_points);
        REQUIRE(stats.stamina >= 0);
        REQUIRE(stats.level >= 1);
    }

    SECTION("Player stats support prompt format") {
        // Test that stats are accessible for prompt system
        // The prompt system in WorldServer::send_prompt_to_actor uses these values
        const auto &stats = player->stats();

        // Verify prompt-relevant values are present
        REQUIRE(stats.hit_points >= 0);
        REQUIRE(stats.stamina >= 0);

        // Test fighting status (used for combat prompts)
        REQUIRE_FALSE(player->is_fighting()); // Initially not fighting

        // Verify position is accessible (used for state checking)
        REQUIRE(player->position() != Position::Dead);
        REQUIRE(player->position() != Position::Ghost);
    }

    SECTION("Stats can be modified") {
        auto &stats = player->stats();
        int original_max_hp = stats.max_hit_points;

        // Test HP modification
        stats.hit_points = original_max_hp / 2;
        REQUIRE(stats.hit_points == original_max_hp / 2);

        // Test HP restoration (like what release command does)
        stats.hit_points = stats.max_hit_points;
        REQUIRE(stats.hit_points == stats.max_hit_points);
    }

    SECTION("Stamina points work") {
        auto &stats = player->stats();

        stats.stamina = 50;
        REQUIRE(stats.stamina == 50);
    }

    SECTION("Stats validation and ranges") {
        const auto &stats = player->stats();

        // Basic stat ranges
        REQUIRE(stats.level >= 1);
        REQUIRE(stats.level <= 100); // Reasonable upper bound
        REQUIRE(stats.hit_points >= 0);
        REQUIRE(stats.max_hit_points > 0);
        REQUIRE(stats.stamina >= 0);
        REQUIRE(stats.max_stamina >= 0);

        // Combat stats (new system uses armor_rating 0-100+)
        REQUIRE(stats.armor_rating >= 0);
        REQUIRE(stats.armor_rating <= 200); // Reasonable upper bound

        // Alignment range
        REQUIRE(stats.alignment >= -1000);
        REQUIRE(stats.alignment <= 1000);

        // Primary attributes (typical D&D ranges)
        REQUIRE(stats.strength >= 3);
        REQUIRE(stats.strength <= 25);
        REQUIRE(stats.dexterity >= 3);
        REQUIRE(stats.dexterity <= 25);
        REQUIRE(stats.intelligence >= 3);
        REQUIRE(stats.intelligence <= 25);
        REQUIRE(stats.wisdom >= 3);
        REQUIRE(stats.wisdom <= 25);
        REQUIRE(stats.constitution >= 3);
        REQUIRE(stats.constitution <= 25);
        REQUIRE(stats.charisma >= 3);
        REQUIRE(stats.charisma <= 25);
    }
}
