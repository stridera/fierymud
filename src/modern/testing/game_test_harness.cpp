/***************************************************************************
 *   File: src/modern/testing/game_test_harness.cpp    Part of FieryMUD *
 *  Usage: Test harness implementation                                      *
 ***************************************************************************/

#include "game_test_harness.hpp"
#include "../core/logging.hpp"
#include "../world/room.hpp"
#include <fmt/format.h>
#include <algorithm>
#include <unordered_map>

GameTestHarness::GameTestHarness() {
    // Initialize world server
    world_server_ = std::make_shared<WorldServer>();
    
    // Get command system from world server
    command_system_ = world_server_->get_command_system();
    
    Log::info("GameTestHarness initialized");
}

Result<std::shared_ptr<Player>> GameTestHarness::create_test_player(
    std::string_view name, 
    int player_class,
    EntityId starting_room
) {
    // Generate unique player ID
    static uint64_t next_id = 10000; // Start high to avoid conflicts
    EntityId player_id{next_id++};
    
    // Create player using factory method
    auto player = Player::create_for_testing(player_id, std::string(name), player_class);
    
    // Set starting room
    player->set_current_room(starting_room);
    
    // Create mock connection for output capture
    auto mock_connection = std::make_shared<MockPlayerConnection>(player_id, this);
    mock_connections_[player_id] = mock_connection;
    
    // Set up player connection interface (we'll need to modify Player class)
    // For now, just add to world server
    world_server_->add_player(player);
    
    // Initialize output capture
    player_output_[player_id] = {};
    
    Log::info("Created test player '{}' (ID: {}) in room {}", 
              name, player_id.value(), starting_room.value());
    
    return player;
}

void GameTestHarness::remove_test_player(std::shared_ptr<Player> player) {
    if (!player) return;
    
    EntityId player_id = player->id();
    
    // Remove from world server
    world_server_->remove_player(player);
    
    // Clean up mock connection and output
    mock_connections_.erase(player_id);
    player_output_.erase(player_id);
    
    Log::info("Removed test player '{}' (ID: {})", 
              player->name(), player_id.value());
}

Result<std::string> GameTestHarness::execute_command(
    std::shared_ptr<Player> player, 
    std::string_view command_line
) {
    if (!player) {
        return std::unexpected(Error{ErrorCode::InvalidArgument, "Player is null"});
    }
    
    // Clear previous output
    clear_output(player);
    
    // Execute command through command system
    auto result = command_system_->process_input(player, command_line);
    
    // Get captured output
    std::string output = get_last_output(player);
    
    if (result != CommandResult::Success) {
        return std::unexpected(Error{ErrorCode::InvalidCommand, 
            fmt::format("Command failed with result: {}", static_cast<int>(result))});
    }
    
    return output;
}

std::shared_ptr<Room> GameTestHarness::get_room(EntityId room_id) const {
    return world_server_->get_room(room_id);
}

std::vector<std::shared_ptr<Player>> GameTestHarness::get_players_in_room(EntityId room_id) const {
    auto room = get_room(room_id);
    if (!room) return {};
    
    std::vector<std::shared_ptr<Player>> players_in_room;
    auto online_players = world_server_->get_online_players();
    
    for (const auto& player : online_players) {
        if (player && player->current_room() == room_id) {
            players_in_room.push_back(player);
        }
    }
    
    return players_in_room;
}

std::vector<std::shared_ptr<Player>> GameTestHarness::get_online_players() const {
    return world_server_->get_online_players();
}

bool GameTestHarness::player_is_in_room(std::shared_ptr<Player> player, EntityId room_id) const {
    return player && player->current_room() == room_id;
}

bool GameTestHarness::player_can_see_player(std::shared_ptr<Player> observer, std::shared_ptr<Player> target) const {
    if (!observer || !target) return false;
    
    // Basic visibility: same room
    return observer->current_room() == target->current_room();
}

std::string GameTestHarness::get_last_output(std::shared_ptr<Player> player) const {
    if (!player) return "";
    
    auto it = player_output_.find(player->id());
    if (it == player_output_.end()) return "";
    
    std::string combined_output;
    for (const auto& line : it->second) {
        if (!combined_output.empty()) combined_output += "\n";
        combined_output += line;
    }
    
    return combined_output;
}

void GameTestHarness::clear_output(std::shared_ptr<Player> player) {
    if (!player) return;
    
    player_output_[player->id()].clear();
}

// MockPlayerConnection implementation
void GameTestHarness::MockPlayerConnection::send_message(std::string_view message) {
    if (harness_) {
        harness_->player_output_[player_id_].emplace_back(message);
    }
}

void GameTestHarness::MockPlayerConnection::send_prompt(std::string_view prompt) {
    if (harness_) {
        harness_->player_output_[player_id_].emplace_back(fmt::format("PROMPT: {}", prompt));
    }
}

void GameTestHarness::MockPlayerConnection::send_line(std::string_view line) {
    if (harness_) {
        harness_->player_output_[player_id_].emplace_back(line);
    }
}