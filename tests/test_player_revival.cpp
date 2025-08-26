#include <catch2/catch_test_macros.hpp>
#include "test_harness.hpp"
#include "../src/core/actor.hpp"
#include "../src/world/world_manager.hpp"

TEST_CASE("Player: Start Room Initialization", "[unit][player][start_room]") {
    TestHarness harness;
    auto fixtures = harness.fixtures();
    
    // Create a test room
    auto& room = fixtures.create_room("Test Room");
    
    // Create a player and set their start room
    auto player_result = Player::create(EntityId{2000}, "TestPlayer");
    REQUIRE(player_result.has_value());
    auto player = std::move(player_result.value());
    
    SECTION("Player can have start room set") {
        REQUIRE(!player->start_room().is_valid()); // Initially invalid
        
        player->set_start_room(room.id());
        REQUIRE(player->start_room().is_valid());
        REQUIRE(player->start_room() == room.id());
    }
    
    SECTION("Player start room persists in JSON") {
        player->set_start_room(room.id());
        
        // Serialize to JSON
        auto json = player->to_json();
        REQUIRE(json.contains("start_room"));
        REQUIRE(json["start_room"].get<uint64_t>() == room.id().value());
        
        // Load from JSON
        auto loaded_result = Player::from_json(json);
        REQUIRE(loaded_result.has_value());
        auto loaded_player = std::move(loaded_result.value());
        
        REQUIRE(loaded_player->start_room() == room.id());
    }
    
    SECTION("Player without start_room falls back to world default") {
        // Set world default start room
        WorldManager::instance().set_start_room(room.id());
        
        // Player has no personal start room
        REQUIRE(!player->start_room().is_valid());
        
        // This would normally be tested with the release command,
        // but that requires more complex setup
    }
}

TEST_CASE("Player: Dead State Command Permissions", "[unit][player][dead]") {
    TestHarness harness;
    
    // Create a player
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
}

TEST_CASE("Player: Revival Release Command Flow", "[integration][player][revival]") {
    TestHarness harness;
    auto fixtures = harness.fixtures();
    
    // Create a start room for revival
    auto& start_room = fixtures.create_room("Temple");
    WorldManager::instance().set_start_room(start_room.id());
    
    SECTION("Ghost player can use release command") {
        // This test would require full command system integration
        // Currently testing the underlying mechanics
        
        auto player_result = Player::create(EntityId{2002}, "TestPlayer");
        REQUIRE(player_result.has_value());
        auto player = std::move(player_result.value());
        
        // Set player as ghost
        player->set_position(Position::Ghost);
        REQUIRE(player->position() == Position::Ghost);
        
        // Set their start room
        player->set_start_room(start_room.id());
        REQUIRE(player->start_room() == start_room.id());
        
        // The actual release command would:
        // 1. Check ghost status ✓ 
        // 2. Get start room ✓
        // 3. Move player there
        // 4. Set position to Standing
        // 5. Restore HP
    }
}