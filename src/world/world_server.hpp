/***************************************************************************
 *   File: src/world/world_server.hpp           Part of FieryMUD *
 *  Usage: World server and game state management                          *
 ***************************************************************************/

#pragma once

#include "../core/ids.hpp"
#include "../game/login_system.hpp"
#include "room.hpp"

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

// Forward declaration
class CommandSystem;

/**
 * @brief Central world server managing game state and logic
 *
 * The WorldServer is responsible for:
 * - Managing all game entities (players, NPCs, objects)
 * - Coordinating world state and updates
 * - Handling player login and character creation
 * - Managing rooms and zones
 */
class WorldServer : public std::enable_shared_from_this<WorldServer> {
  public:
    WorldServer();
    ~WorldServer() = default;

    // Character management functions - used by LoginSystem
    std::shared_ptr<class Player> create_character(const std::string &name);
    bool save_character(std::shared_ptr<class Player> player);
    std::shared_ptr<class Player> load_character(const std::string &name);

    // Command system access
    std::shared_ptr<CommandSystem> get_command_system() const;

    // World data access
    std::shared_ptr<Room> get_room(EntityId room_id) const;
    const std::unordered_map<EntityId, std::shared_ptr<Room>> &get_all_rooms() const { return rooms_; }

    // Player management
    void add_player(std::shared_ptr<class Player> player);
    void remove_player(std::shared_ptr<class Player> player);
    std::vector<std::shared_ptr<class Player>> get_online_players() const;

    // World initialization
    void initialize_world();
    bool load_world_data();

    // Game loop and updates
    void update();

    // Player starting location
    EntityId starting_room() const { return EntityId{static_cast<uint64_t>(starting_room_vnum_)}; }

  private:
    void load_test_world();
    void create_starting_room();

    // Core systems
    mutable std::shared_ptr<CommandSystem> command_system_;

    // World data
    std::unordered_map<EntityId, std::shared_ptr<Room>> rooms_;
    std::vector<std::shared_ptr<class Player>> online_players_;

    // World state
    bool world_loaded_ = false;
    int starting_room_vnum_ = 3001;
};