#pragma once

#include "../core/ids.hpp"
#include "../core/result.hpp"
#include "../world/game_world.hpp"

#include <asio.hpp>

#include <memory>
#include <string>
#include <string_view>
#include <vector>

/**
 * @brief Test harness for game functionality without networking
 *
 * Provides direct access to game systems for testing:
 * - Create players without connections
 * - Execute commands directly
 * - Verify game state
 * - Test complete game flows
 */
class GameTestHarness {
  public:
    GameTestHarness();
    ~GameTestHarness() = default;

    // Player management
    Result<std::shared_ptr<Player>> create_test_player(std::string_view name,
                                                       int player_class = 1, // 1=Warrior, 2=Cleric, 3=Sorcerer, 4=Rogue
                                                       EntityId starting_room = EntityId{1001});

    void remove_test_player(std::shared_ptr<Player> player);

    // Command execution
    Result<std::string> execute_command(std::shared_ptr<Player> player, std::string_view command_line);

    // World state queries
    std::shared_ptr<Room> get_room(EntityId room_id) const;
    std::vector<std::shared_ptr<Player>> get_players_in_room(EntityId room_id) const;
    std::vector<std::shared_ptr<Player>> get_online_players() const;

    // Game state verification helpers
    bool player_is_in_room(std::shared_ptr<Player> player, EntityId room_id) const;
    bool player_can_see_player(std::shared_ptr<Player> observer, std::shared_ptr<Player> target) const;

    // World server access
    std::shared_ptr<GameWorld> world_server() const { return world_server_; }

    // Output capture for testing
    std::string get_last_output(std::shared_ptr<Player> player) const;
    void clear_output(std::shared_ptr<Player> player);

  private:
    asio::io_context io_context_;
    std::shared_ptr<GameWorld> world_server_;
    std::shared_ptr<CommandSystem> command_system_;

    // Test player output capture
    std::unordered_map<EntityId, std::vector<std::string>> player_output_;

    // Mock connection interface for players
    class MockPlayerConnection;
    std::unordered_map<EntityId, std::shared_ptr<MockPlayerConnection>> mock_connections_;
};

/**
 * @brief Mock connection that captures output instead of sending to network
 */
class GameTestHarness::MockPlayerConnection {
  public:
    explicit MockPlayerConnection(EntityId player_id, GameTestHarness *harness)
        : player_id_(player_id), harness_(harness) {}

    void send_message(std::string_view message);
    void send_prompt(std::string_view prompt);
    void send_line(std::string_view line);

    // GMCP methods (no-op for testing)
    void send_room_info() {}
    void send_char_vitals() {}

    std::string remote_address() const { return "test-harness"; }

  private:
    EntityId player_id_;
    GameTestHarness *harness_;
};
