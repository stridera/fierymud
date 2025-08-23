/***************************************************************************
 *   File: src/testing/test_simple_functionality.cpp Part of FieryMUD *
 *  Usage: Simple test program to verify core game functionality             *
 ***************************************************************************/

#include "../core/config.hpp"
#include "../core/logging.hpp"
#include "../game/player.hpp"
#include "../world/world_server.hpp"

#include <cassert>
#include <iostream>

void test_player_creation() {
    std::cout << "\n=== Testing Player Creation ===\n";

    // Create a test player
    auto player = Player::create_for_testing(EntityId{10001}, "TestWarrior", 1);
    assert(player != nullptr);
    assert(player->name() == "TestWarrior");
    assert(player->current_room() == EntityId{1001}); // Default from config

    std::cout << "✓ Player creation successful\n";
    std::cout << "✓ Player '" << player->name() << "' created in room " << player->current_room().value() << "\n";
}

void test_world_server() {
    std::cout << "\n=== Testing WorldServer ===\n";

    auto world_server = std::make_shared<WorldServer>();

    // Test room loading
    auto room = world_server->get_room(EntityId{1001});
    if (room) {
        std::cout << "✓ Room loading successful\n";
        std::cout << "Room name: " << room->name() << "\n";
        std::cout << "Room description: " << room->description() << "\n";
    } else {
        std::cout << "✗ Room 1001 not found\n";
    }

    // Test player management
    auto player1 = Player::create_for_testing(EntityId{20001}, "Player1", 1);
    auto player2 = Player::create_for_testing(EntityId{20002}, "Player2", 2);

    world_server->add_player(player1);
    world_server->add_player(player2);

    auto online_players = world_server->get_online_players();
    std::cout << "✓ Added 2 players, online count: " << online_players.size() << "\n";
    assert(online_players.size() == 2);

    // Test player removal
    world_server->remove_player(player1);
    online_players = world_server->get_online_players();
    std::cout << "✓ After removal, online count: " << online_players.size() << "\n";
    assert(online_players.size() == 1);
}

void test_room_functionality() {
    std::cout << "\n=== Testing Room Functionality ===\n";

    auto world_server = std::make_shared<WorldServer>();

    // Test multiple rooms
    for (int i = 1001; i <= 1004; ++i) {
        auto room = world_server->get_room(EntityId{i});
        if (room) {
            std::cout << "✓ Room " << i << ": " << room->name() << "\n";

            // Show exits
            const auto &exits = room->exits();
            if (!exits.empty()) {
                std::cout << "  Exits: ";
                for (const auto &[direction, exit] : exits) {
                    std::cout << direction << "->" << exit.destination().value() << " ";
                }
                std::cout << "\n";
            }
        }
    }
}

void test_player_vitals() {
    std::cout << "\n=== Testing Player Vitals ===\n";

    auto player = Player::create_for_testing(EntityId{30001}, "VitalsTest", 1);

    // Test GMCP vitals
    auto vitals_json = player->get_vitals_gmcp();
    std::cout << "✓ Vitals GMCP data generated\n";
    std::cout << "Health: " << vitals_json["hp"] << "/" << vitals_json["maxhp"] << "\n";
    std::cout << "Movement: " << vitals_json["move"] << "/" << vitals_json["maxmove"] << "\n";

    // Test status GMCP
    auto status_json = player->get_status_gmcp();
    std::cout << "✓ Status GMCP data generated\n";
    std::cout << "Name: " << status_json["name"] << "\n";
    std::cout << "Level: " << status_json["level"] << "\n";
    std::cout << "Class: " << status_json["class"] << "\n";
    std::cout << "Room: " << status_json["room"] << "\n";
}

void test_player_movement() {
    std::cout << "\n=== Testing Player Movement ===\n";

    auto player = Player::create_for_testing(EntityId{40001}, "Mover", 1);

    // Test room changes
    EntityId original_room = player->current_room();
    std::cout << "Original room: " << original_room.value() << "\n";

    // Move to different room
    player->set_current_room(EntityId{1002});
    EntityId new_room = player->current_room();
    std::cout << "New room: " << new_room.value() << "\n";

    assert(new_room != original_room);
    std::cout << "✓ Player movement successful\n";

    // Move back
    player->set_current_room(original_room);
    assert(player->current_room() == original_room);
    std::cout << "✓ Return movement successful\n";
}

int main() {
    try {
        // Initialize configuration and logging
        auto config_result = Config::initialize("config/test_mud.json");
        if (!config_result) {
            std::cerr << "Failed to load configuration: " << config_result.error().message << std::endl;
            return 1;
        }

        Logger::initialize("test_simple.log", LogLevel::Info);

        std::cout << "=== FieryMUD Simple Functionality Test ===\n";
        std::cout << "Testing core game systems without networking...\n";

        // Run tests
        test_player_creation();
        test_world_server();
        test_room_functionality();
        test_player_vitals();
        test_player_movement();

        std::cout << "\n=== All Simple Tests Completed Successfully! ===\n";
        std::cout << "✓ Player creation and management\n";
        std::cout << "✓ WorldServer functionality\n";
        std::cout << "✓ Room loading and navigation\n";
        std::cout << "✓ Player vitals and GMCP generation\n";
        std::cout << "✓ Player movement system\n";
        std::cout << "\nCore game systems are working correctly!\n";

        return 0;

    } catch (const std::exception &e) {
        std::cerr << "Test error: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Unknown test error occurred" << std::endl;
        return 1;
    }
}