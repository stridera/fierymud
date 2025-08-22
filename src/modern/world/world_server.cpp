/***************************************************************************
 *   File: src/modern/world/world_server.cpp           Part of FieryMUD *
 *  Usage: World server and game state management                          *
 ***************************************************************************/

#include "world_server.hpp"
#include "../core/logging.hpp"
#include "../core/actor.hpp"
#include "../commands/command_system.hpp"
#include <algorithm>

WorldServer::WorldServer() {
    Log::info("WorldServer initializing...");
    initialize_world();
}

void WorldServer::initialize_world() {
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

bool WorldServer::load_world_data() {
    // For now, create a minimal test world
    load_test_world();
    return true;
}

void WorldServer::load_test_world() {
    Log::debug("Loading test world data...");
    
    // Create Temple of Midgaard (starting room)
    auto temple = std::make_shared<Room>(EntityId{3001UL}, "The Temple of Midgaard");
    temple->set_zone_id(EntityId{30UL});
    temple->set_sector_type(SectorType::Inside);
    temple->set_description(
        "You are in the southern end of the temple hall in the Temple of "
        "Midgaard. The temple has been constructed from giant marble blocks, "
        "eternal in appearance, and most of the walls are covered by ancient "
        "wall paintings depicting gods, giants and peasants."
    );
    
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

void WorldServer::create_starting_room() {
    Log::info("Creating default starting room...");
    
    auto room = std::make_shared<Room>(EntityId{static_cast<uint64_t>(starting_room_vnum_)}, "Default Starting Room");
    room->set_zone_id(EntityId{1UL});
    room->set_sector_type(SectorType::Inside);
    room->set_description("This is a default starting room for the modern FieryMUD server.");
    
    rooms_[EntityId{static_cast<uint64_t>(starting_room_vnum_)}] = room;
}

std::shared_ptr<Room> WorldServer::get_room(EntityId room_id) const {
    auto it = rooms_.find(room_id);
    if (it != rooms_.end()) {
        return it->second;
    }
    
    Log::debug("Room {} not found", room_id.value());
    return nullptr;
}

void WorldServer::add_player(std::shared_ptr<Player> player) {
    if (!player) return;
    
    // Check if player is already in the list
    auto it = std::find(online_players_.begin(), online_players_.end(), player);
    if (it == online_players_.end()) {
        online_players_.push_back(player);
        Log::info("Player '{}' added to world. {} players online.", 
                  player->name(), online_players_.size());
    }
}

void WorldServer::remove_player(std::shared_ptr<Player> player) {
    if (!player) return;
    
    auto it = std::find(online_players_.begin(), online_players_.end(), player);
    if (it != online_players_.end()) {
        online_players_.erase(it);
        Log::info("Player '{}' removed from world. {} players online.", 
                  player->name(), online_players_.size());
    }
}

std::vector<std::shared_ptr<Player>> WorldServer::get_online_players() const {
    return online_players_;
}

void WorldServer::update() {
    // World tick updates would go here
    // For now, this is a placeholder for:
    // - NPC AI updates
    // - Zone resets
    // - Timed events
    // - Spell effects
    // - etc.
}

std::shared_ptr<Player> WorldServer::create_character(const std::string& name) {
    // Use Player::create factory method since constructor is private
    auto result = Player::create(
        EntityId{static_cast<uint64_t>(online_players_.size() + 1000)}, 
        name, 
        1  // Default class (warrior)
    );
    
    if (!result) {
        Log::error("Failed to create character: {}", name);
        return nullptr;
    }
    
    auto player = std::shared_ptr<Player>(result.value().release());
    
    // Set starting location
    player->set_current_room(EntityId{static_cast<uint64_t>(starting_room_vnum_)});
    
    Log::info("Created new character: {}", name);
    return player;
}

bool WorldServer::save_character(std::shared_ptr<Player> player) {
    if (!player) return false;
    
    // For now, just log that we would save
    Log::info("Saving character: {}", player->name());
    
    // TODO: Implement actual character persistence
    return true;
}

std::shared_ptr<Player> WorldServer::load_character(const std::string& name) {
    // For now, return nullptr (character doesn't exist)
    Log::debug("Attempting to load character: {}", name);
    
    // TODO: Implement actual character loading
    return nullptr;
}

std::shared_ptr<CommandSystem> WorldServer::get_command_system() const {
    if (!command_system_) {
        // Lazy initialization - safe because this is called after construction
        command_system_ = std::make_shared<CommandSystem>(
            std::const_pointer_cast<WorldServer>(shared_from_this())
        );
        Log::info("Command system initialized");
    }
    return command_system_;
}