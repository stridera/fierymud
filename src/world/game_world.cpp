/***************************************************************************
 *   File: src/world/world_server.cpp           Part of FieryMUD *
 *  Usage: World server and game state management                          *
 ***************************************************************************/

#include "game_world.hpp"

#include "../commands/command_system.hpp"
#include "../core/actor.hpp"
#include "../core/combat.hpp"
#include "../core/logging.hpp"

#include <algorithm>

GameWorld::GameWorld(asio::io_context& io_context) : io_context_(io_context) {
    Log::info("GameWorld initializing...");
    update_timer_ = std::make_unique<asio::steady_timer>(io_context_);
    initialize_world();
}

void GameWorld::initialize_world() {
    Log::info("Initializing world data...");

    // Load basic world data
    load_world_data();

    // Create starting room if needed
    if (rooms_.empty()) {
        create_starting_room();
    }

    world_loaded_ = true;
    Log::info("World initialization complete. {} rooms loaded.", rooms_.size());
}

bool GameWorld::load_world_data() {
    // For now, create a minimal test world
    load_test_world();
    return true;
}

void GameWorld::load_test_world() {
    Log::debug("Loading test world data...");

    // Create Temple of Midgaard (starting room)
    auto temple = std::make_shared<Room>(EntityId{3001UL}, "The Temple of Midgaard");
    temple->set_zone_id(EntityId{30UL});
    temple->set_sector_type(SectorType::Inside);
    temple->set_description(
        "You are in the southern end of the temple hall in the Temple of "
        "Midgaard. The temple has been constructed from giant marble blocks, "
        "eternal in appearance, and most of the walls are covered by ancient "
        "wall paintings depicting gods, giants and peasants.");

    // Add some exits for testing
    temple->add_exit("north", EntityId{3002UL});
    temple->add_exit("east", EntityId{3003UL});
    temple->add_exit("west", EntityId{3004UL});

    rooms_[EntityId{3001UL}] = temple;

    // Create a few more test rooms
    auto north_hall = std::make_shared<Room>(EntityId{3002UL}, "Northern Temple Hall");
    north_hall->set_zone_id(EntityId{30UL});
    north_hall->set_sector_type(SectorType::Inside);
    north_hall->set_description("The northern end of the temple hall.");
    north_hall->add_exit("south", EntityId{3001UL});
    rooms_[EntityId{3002UL}] = north_hall;

    auto east_room = std::make_shared<Room>(EntityId{3003UL}, "Eastern Chamber");
    east_room->set_zone_id(EntityId{30UL});
    east_room->set_sector_type(SectorType::Inside);
    east_room->set_description("A small chamber to the east of the temple.");
    east_room->add_exit("west", EntityId{3001UL});
    rooms_[EntityId{3003UL}] = east_room;

    auto west_room = std::make_shared<Room>(EntityId{3004UL}, "Western Chamber");
    west_room->set_zone_id(EntityId{30UL});
    west_room->set_sector_type(SectorType::Inside);
    west_room->set_description("A small chamber to the west of the temple.");
    west_room->add_exit("east", EntityId{3001UL});
    rooms_[EntityId{3004UL}] = west_room;

    Log::debug("Test world loaded with {} rooms", rooms_.size());
}

void GameWorld::create_starting_room() {
    Log::info("Creating default starting room...");

    auto room = std::make_shared<Room>(starting_room_id_, "Default Starting Room");
    room->set_zone_id(EntityId{1UL});
    room->set_sector_type(SectorType::Inside);
    room->set_description("This is a default starting room for the modern FieryMUD server.");
    // Ensure starting room is always well lit
    room->set_light_level(5);

    rooms_[starting_room_id_] = room;
}

std::shared_ptr<Room> GameWorld::get_room(EntityId room_id) const {
    auto it = rooms_.find(room_id);
    if (it != rooms_.end()) {
        return it->second;
    }

    Log::debug("Room {} not found", room_id.value());
    return nullptr;
}

void GameWorld::add_player(std::shared_ptr<Player> player) {
    if (!player)
        return;

    // Check if player is already in the list
    auto it = std::find(online_players_.begin(), online_players_.end(), player);
    if (it == online_players_.end()) {
        online_players_.push_back(player);
        Log::info("Player '{}' added to world. {} players online.", player->name(), online_players_.size());
    }
}

void GameWorld::remove_player(std::shared_ptr<Player> player) {
    if (!player)
        return;

    auto it = std::find(online_players_.begin(), online_players_.end(), player);
    if (it != online_players_.end()) {
        online_players_.erase(it);
        Log::info("Player '{}' removed from world. {} players online.", player->name(), online_players_.size());
    }
}

std::vector<std::shared_ptr<Player>> GameWorld::get_online_players() const { return online_players_; }

void GameWorld::update() {
    // Process ongoing combat rounds
    FieryMUD::CombatManager::process_combat_rounds();
    
    // World tick updates would go here
    // For now, this is a placeholder for:
    // - NPC AI updates
    // - Zone resets
    // - Timed events (combat now implemented!)
    // - Spell effects
    // - etc.
}

void GameWorld::start_game_loop() {
    if (game_loop_running_) {
        Log::warn("Game loop already running");
        return;
    }
    
    Log::info("Starting game loop with {}ms update interval", update_interval_.count());
    game_loop_running_ = true;
    schedule_next_update();
}

void GameWorld::stop_game_loop() {
    if (!game_loop_running_) {
        return;
    }
    
    Log::info("Stopping game loop");
    game_loop_running_ = false;
    
    if (update_timer_) {
        update_timer_->cancel();
    }
}

void GameWorld::schedule_next_update() {
    if (!game_loop_running_ || !update_timer_) {
        return;
    }
    
    update_timer_->expires_after(update_interval_);
    auto self = shared_from_this();
    update_timer_->async_wait([this, self](const std::error_code& error) {
        handle_update_timer(error);
    });
}

void GameWorld::handle_update_timer(const std::error_code& error) {
    if (error == asio::error::operation_aborted) {
        // Timer was cancelled, normal during shutdown
        return;
    }
    
    if (error) {
        Log::error("Game loop timer error: {}", error.message());
        return;
    }
    
    if (!game_loop_running_) {
        return;
    }
    
    try {
        // Call the update method which processes combat rounds
        update();
    } catch (const std::exception& e) {
        Log::error("Error during world update: {}", e.what());
    }
    
    // Schedule the next update
    schedule_next_update();
}

std::shared_ptr<Player> GameWorld::create_character(const std::string &name) {
    // Use Player::create factory method since constructor is private
    auto result = Player::create(EntityId{static_cast<uint64_t>(online_players_.size() + 1000)}, name,
                                 1 // Default class (warrior)
    );

    if (!result) {
        Log::error("Failed to create character: {}", name);
        return nullptr;
    }

    auto player = std::shared_ptr<Player>(result.value().release());

    // Set starting location
    player->set_current_room(starting_room_id_);

    Log::info("Created new character: {}", name);
    return player;
}

bool GameWorld::save_character(std::shared_ptr<Player> player) {
    if (!player)
        return false;

    // For now, just log that we would save
    Log::info("Saving character: {}", player->name());

    // TODO: Implement actual character persistence
    return true;
}

std::shared_ptr<Player> GameWorld::load_character(const std::string &name) {
    // For now, return nullptr (character doesn't exist)
    Log::debug("Attempting to load character: {}", name);

    // TODO: Implement actual character loading
    return nullptr;
}

std::shared_ptr<CommandSystem> GameWorld::get_command_system() const {
    if (!command_system_) {
        // Lazy initialization - safe because this is called after construction
        command_system_ = std::make_shared<CommandSystem>(std::const_pointer_cast<GameWorld>(shared_from_this()));
        Log::info("Command system initialized");
    }
    return command_system_;
}