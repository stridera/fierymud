#pragma once

#include "../core/ids.hpp"

#include <asio.hpp>
#include <chrono>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

// Forward declaration
class CommandSystem;
class Room;
class Player;

/**
 * @brief Central world server managing game state and logic
 *
 * The WorldServer is responsible for:
 * - Managing all game entities (players, NPCs, objects)
 * - Coordinating world state and updates
 * - Handling player login and character creation
 * - Managing rooms and zones
 */
class GameWorld : public std::enable_shared_from_this<GameWorld> {
  public:
    explicit GameWorld(asio::io_context& io_context);
    ~GameWorld() = default;

    // Character management functions - used by LoginSystem
    std::shared_ptr<Player> create_character(const std::string &name);
    bool save_character(std::shared_ptr<Player> player);
    std::shared_ptr<Player> load_character(const std::string &name);

    // Command system access
    std::shared_ptr<CommandSystem> get_command_system() const;

    // World data access
    std::shared_ptr<Room> get_room(EntityId room_id) const;
    const std::unordered_map<EntityId, std::shared_ptr<Room>> &get_all_rooms() const { return rooms_; }

    // Player management
    void add_player(std::shared_ptr<Player> player);
    void remove_player(std::shared_ptr<Player> player);
    std::vector<std::shared_ptr<Player>> get_online_players() const;

    // World initialization and lifecycle
    void initialize_world();
    bool load_world_data();
    void start_game_loop();
    void stop_game_loop();

    // Game loop and updates
    void update();

    // Player starting location
    EntityId starting_room() const { return starting_room_id_; }

  private:
    void load_test_world();
    void create_starting_room();
    void schedule_next_update();
    void handle_update_timer(const std::error_code& error);

    // Core systems
    mutable std::shared_ptr<CommandSystem> command_system_;

    // Asio context and timer for game loop
    asio::io_context& io_context_;
    std::unique_ptr<asio::steady_timer> update_timer_;
    std::chrono::milliseconds update_interval_{100}; // 10 TPS (ticks per second)
    bool game_loop_running_ = false;

    // World data
    std::unordered_map<EntityId, std::shared_ptr<Room>> rooms_;
    std::vector<std::shared_ptr<Player>> online_players_;

    // World state
    bool world_loaded_ = false;
    EntityId starting_room_id_{3001UL};
};
