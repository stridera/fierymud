/***************************************************************************
 *   File: src/world/world_manager.cpp                    Part of FieryMUD *
 *  Usage: World management system implementation                          *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.       *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 ***************************************************************************/

#include "world_manager.hpp"
#include "weather.hpp"
#include "../core/actor.hpp"
#include "../core/object.hpp"
#include "../core/logging.hpp"

#include <filesystem>
#include <fstream>
#include <algorithm>
#include <queue>
#include <unordered_set>

// WorldManager Implementation

WorldManager& WorldManager::instance() {
    static WorldManager instance;
    return instance;
}

Result<void> WorldManager::initialize(const std::string& world_path, bool load_from_files) {
    std::unique_lock lock(world_mutex_);
    
    if (initialized_.load()) {
        return std::unexpected(Errors::InvalidState("WorldManager already initialized"));
    }
    
    world_path_ = world_path;
    
    // Validate world directory structure
    if (!WorldUtils::validate_world_directory(world_path_)) {
        auto logger = Log::game();
        logger->warn("World directory structure invalid, attempting to create: {}", world_path_);
        
        TRY(WorldUtils::create_world_directory(world_path_));
    }
    
    stats_.startup_time = std::chrono::steady_clock::now();
    stats_.last_save = stats_.startup_time;
    
    initialized_.store(true);
    
    auto logger = Log::game();
    logger->info("WorldManager initialized with world path: {}", world_path_);

    if (load_from_files) {
        auto world_load = load_world();
        if (!world_load) {
            Log::error("Failed to load world data during initialization: {}", world_load.error().message);
            return std::unexpected(world_load.error());
        }
    }
    
    return Success();
}

void WorldManager::shutdown() {
    std::unique_lock lock(world_mutex_);
    
    if (!initialized_.load()) {
        return;
    }
    
    auto logger = Log::game();
    logger->info("Shutting down WorldManager");
      
    clear_state();
    
    initialized_.store(false);
    has_unsaved_changes_.store(false);
    
    logger->info("WorldManager shutdown complete");
}

void WorldManager::clear_state() {
    std::unique_lock lock(world_mutex_);

    // Clear all data
    rooms_.clear();
    zones_.clear();
    object_prototypes_.clear();
    objects_.clear();
    mobile_prototypes_.clear();
    mobiles_.clear();
    spawned_mobiles_.clear();  // Clear mobile instance tracking
    scheduled_resets_.clear();
    file_timestamps_.clear();
    start_room_ = INVALID_ENTITY_ID;
    stats_ = WorldStats{}; // Reset stats
    has_unsaved_changes_.store(false);

    // Clear any active actors in rooms (if any remain)
    // This is important for tests to ensure no actors persist between runs
    for (const auto& [id, room] : rooms_) {
        if (room) {
            room->contents_mutable().actors.clear();
        }
    }

    // Clear weather system state
    WeatherSystem::instance().shutdown();
    WeatherSystem::instance().initialize();

    // Reset initialized state to allow re-initialization (important for tests)
    initialized_.store(false);

    Log::game()->debug("WorldManager state cleared");
}

Result<void> WorldManager::load_world() {
    // Note: This method is called from the world strand - no mutex needed!
    // The strand provides single-threaded access by design.
    
    if (!initialized_.load()) {
        return std::unexpected(Errors::InvalidState("WorldManager not initialized"));
    }
    
    auto start_time = std::chrono::steady_clock::now();
    auto logger = Log::game();
    
    logger->info("Loading world from: {}", world_path_);
    
    // Clear existing data
    rooms_.clear();
    zones_.clear();
    stats_ = WorldStats{};
    stats_.startup_time = start_time;
    
    // Load zones with embedded rooms, objects, and mobs directly from world directory
    TRY(load_zones_from_directory(world_path_));
    
    // Rooms are now loaded from embedded zone data, no separate room directory needed
    
    // Update statistics
    auto end_time = std::chrono::steady_clock::now();
    stats_.load_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    logger->info("World loading complete: {} zones, {} rooms loaded in {}ms",
                stats_.zones_loaded, stats_.rooms_loaded, stats_.load_time.count());
    
    // Validate world integrity
    auto validation = validate_world();
    if (!validation.is_valid) {
        logger->warn("World validation found issues: {}", validation.summary());
        for (const auto& error : validation.errors) {
            logger->error("World validation error: {}", error);
        }
        for (const auto& warning : validation.warnings) {
            logger->warn("World validation warning: {}", warning);
        }
    }
    
    update_file_timestamps();
    has_unsaved_changes_.store(false);
    
    // Initialize weather system for loaded zones
    initialize_weather_callbacks();
    
    // Force initial zone resets to spawn mobiles and populate the world
    logger->info("Performing initial zone resets to populate world...");
    for (const auto& [zone_id, zone] : zones_) {
        if (zone) {
            logger->debug("Force resetting zone {} ({}) for initial population", zone_id, zone->name());
            zone->force_reset();
        }
    }
    logger->info("Initial zone resets completed - world populated with mobiles and objects");
    
    return Success();
}

Result<void> WorldManager::load_zone_file(const std::string& filename) {
    auto logger = Log::game();
    logger->debug("Loading zone file: {}", filename);
    
    // Load JSON zone file
    std::ifstream file(filename);
    if (!file.is_open()) {
        stats_.failed_zone_loads++;
        return std::unexpected(Errors::FileNotFound(filename));
    }
    
    nlohmann::json zone_json;
    try {
        file >> zone_json;
    } catch (const nlohmann::json::exception& e) {
        stats_.failed_zone_loads++;
        return std::unexpected(Errors::ParseError("Zone JSON", e.what()));
    }
    
    // Extract zone data from the nested JSON structure
    nlohmann::json zone_data;
    if (zone_json.contains("zone") && zone_json["zone"].is_object()) {
        zone_data = zone_json["zone"];
        
        // Convert numeric ID to proper format expected by modern system
        if (zone_data.contains("id") && zone_data["id"].is_string()) {
            try {
                auto id_value = std::stoull(zone_data["id"].get<std::string>());
                // Convert 0 to 1 since the modern system doesn't like ID 0
                if (id_value == 0) {
                    id_value = 1000;  // Use a high number for zone 0 to avoid conflicts
                }
                zone_data["id"] = id_value;
            } catch (const std::exception& e) {
                stats_.failed_zone_loads++;
                return std::unexpected(Errors::ParseError("Zone ID conversion", e.what()));
            }
        }
    } else {
        // Fall back to direct zone_json if it's in the modern format
        zone_data = zone_json;
    }
    
    // Create the zone first - pass the full JSON structure
    auto zone_result = Zone::from_json(zone_json);
    if (!zone_result) {
        stats_.failed_zone_loads++;
        return std::unexpected(zone_result.error());
    }
    
    auto zone = std::move(zone_result.value());
    auto zone_id = zone->id();
    auto zone_name = zone->name();
    
    // Add zone to the registry first so it's available for spawning
    auto zone_shared = std::shared_ptr<Zone>(zone.release());
    zones_[zone_id] = zone_shared;
    
    // Set up zone reset callbacks
    setup_zone_callbacks(zone_shared);
    
    // Parse embedded rooms and add them to the world
    if (zone_json.contains("rooms") && zone_json["rooms"].is_array()) {
        for (const auto& room_json : zone_json["rooms"]) {
            // Handle both room objects and room ID references
            if (room_json.is_object()) {
                // Room object with full data
                auto room_result = Room::from_json(room_json);
                if (room_result) {
                    auto room = std::shared_ptr<Room>(room_result.value().release());
                    
                    // Set the room's zone reference
                    room->set_zone_id(zone_id);
                    
                    // Add room to world manager
                    rooms_[room->id()] = room;
                    stats_.rooms_loaded++;
                    
                    // Add room ID to zone's room list
                    zone_shared->add_room(room->id());
                    
                    logger->debug("Loaded room: {} ({}) in zone {}", room->name(), room->id(), zone_name);
                } else {
                    logger->error("Failed to parse room in zone {}: {}", zone_name, room_result.error().message);
                }
            } else if (room_json.is_number()) {
                // Room ID reference - just add to zone's room list
                EntityId room_id{room_json.get<std::uint64_t>()};
                zone_shared->add_room(room_id);
                logger->debug("Referenced room: {} in zone {}", room_id, zone_name);
            } else {
                logger->warn("Invalid room entry in zone {}: expected object or number", zone_name);
            }
        }
    }
    
    // Parse embedded objects
    if (zone_json.contains("objects") && zone_json["objects"].is_array()) {
        for (const auto& object_json : zone_json["objects"]) {
            auto object_result = Object::from_json(object_json);
            if (object_result) {
                auto object = std::move(object_result.value());
                
                // Add object prototype to registry
                objects_[object->id()] = object.get();
                object_prototypes_.push_back(std::move(object));
                stats_.objects_loaded++;
                
                logger->debug("Loaded object prototype: {} ({}) in zone {}", 
                             object_prototypes_.back()->name(), object_prototypes_.back()->id(), zone_name);
            } else {
                logger->error("Failed to parse object in zone {}: {}", zone_name, object_result.error().message);
            }
        }
    }
    
    // Parse mobiles from the "mobs" array to create prototypes (but don't spawn them yet)
    if (zone_json.contains("mobs") && zone_json["mobs"].is_array()) {
        for (const auto& mob_json : zone_json["mobs"]) {
            if (!mob_json.contains("id")) {
                logger->warn("Mobile in zone {} missing id field", zone_name);
                continue;
            }
            
            EntityId mob_id{mob_json["id"].get<std::uint64_t>()};
            
            // Check if we already have this mobile prototype
            if (mobiles_.find(mob_id) != mobiles_.end()) {
                continue;
            }
            
            // Create mobile prototype from JSON data
            auto mob_result = Mobile::from_json(mob_json);
            if (mob_result) {
                auto mob = std::move(mob_result.value());
                mobiles_[mob->id()] = mob.get();
                mobile_prototypes_.push_back(std::move(mob));
                stats_.mobiles_loaded++;
                
                logger->debug("Loaded mobile prototype: {} ({}) in zone {}", 
                             mobile_prototypes_.back()->name(), mob_id, zone_name);
            } else {
                logger->error("Failed to parse mobile {} in zone {}: {}", 
                             mob_id, zone_name, mob_result.error().message);
            }
        }
    }
    
    // Note: Mobile spawning is now handled entirely by zone reset commands
    // The zone.commands.mob array will be processed by Zone::parse_nested_zone_commands()
    // and spawning will occur during zone->force_reset() in the main world loading loop
    
    // Note: This method is called from the world strand - no mutex needed!
    // Zone already added above before processing embedded content
    stats_.zones_loaded++;
    has_unsaved_changes_.store(true);
    
    logger->info("Loaded zone: {} ({})", zone_name, zone_id);
    
    return Success();
}

Result<void> WorldManager::reload_zone(EntityId zone_id) {
    std::shared_lock read_lock(world_mutex_);
    auto zone_it = zones_.find(zone_id);
    if (zone_it == zones_.end()) {
        return std::unexpected(Errors::NotFound("zone"));
    }
    
    auto zone = zone_it->second;
    read_lock.unlock();
    
    // Determine zone file path
    std::string filename = fmt::format("{}/{}.json", world_path_, zone_id.value());
    
    auto result = zone->reload_from_file(filename);
    if (result) {
        has_unsaved_changes_.store(true);
        auto logger = Log::game();
        logger->info("Reloaded zone: {} ({})", zone->name(), zone_id);
    }
    
    return result;
}

Result<void> WorldManager::reload_all_zones() {
    std::shared_lock read_lock(world_mutex_);
    std::vector<EntityId> zone_ids;
    for (const auto& [id, zone] : zones_) {
        zone_ids.push_back(id);
    }
    read_lock.unlock();
    
    for (EntityId zone_id : zone_ids) {
        auto result = reload_zone(zone_id);
        if (!result) {
            auto logger = Log::game();
            logger->error("Failed to reload zone {}: {}", zone_id, result.error().message);
        }
    }
    
    return Success();
}

std::shared_ptr<Room> WorldManager::get_room(EntityId room_id) const {
    std::shared_lock lock(world_mutex_);
    auto it = rooms_.find(room_id);
    return it != rooms_.end() ? it->second : nullptr;
}

std::shared_ptr<Room> WorldManager::get_first_available_room() const {
    std::shared_lock lock(world_mutex_);
    // Return the first available room, preferring lower-numbered rooms
    if (!rooms_.empty()) {
        return rooms_.begin()->second;
    }
    return nullptr;
}

Result<void> WorldManager::add_room(std::shared_ptr<Room> room) {
    if (!room) {
        return std::unexpected(Errors::InvalidArgument("room", "cannot be null"));
    }
    
    std::unique_lock lock(world_mutex_);
    rooms_[room->id()] = room;
    has_unsaved_changes_.store(true);
    
    auto logger = Log::game();
    logger->debug("Added room: {} ({})", room->name(), room->id());
    
    return Success();
}

void WorldManager::remove_room(EntityId room_id) {
    std::unique_lock lock(world_mutex_);
    
    auto it = rooms_.find(room_id);
    if (it != rooms_.end()) {
        auto logger = Log::game();
        logger->debug("Removing room: {} ({})", it->second->name(), room_id);
        
        rooms_.erase(it);
        has_unsaved_changes_.store(true);
    }
}

std::vector<std::shared_ptr<Room>> WorldManager::find_rooms_by_keyword(std::string_view keyword) const {
    std::shared_lock lock(world_mutex_);
    std::vector<std::shared_ptr<Room>> results;
    
    for (const auto& [id, room] : rooms_) {
        if (room && room->matches_keyword(keyword)) {
            results.push_back(room);
        }
    }
    
    return results;
}

std::vector<std::shared_ptr<Room>> WorldManager::get_rooms_in_zone(EntityId zone_id) const {
    std::shared_lock lock(world_mutex_);
    std::vector<std::shared_ptr<Room>> results;
    
    for (const auto& [id, room] : rooms_) {
        if (room && room->zone_id() == zone_id) {
            results.push_back(room);
        }
    }
    
    return results;
}

std::shared_ptr<Zone> WorldManager::get_zone(EntityId zone_id) const {
    std::shared_lock lock(world_mutex_);
    auto it = zones_.find(zone_id);
    return it != zones_.end() ? it->second : nullptr;
}

Result<void> WorldManager::add_zone(std::shared_ptr<Zone> zone) {
    if (!zone) {
        return std::unexpected(Errors::InvalidArgument("zone", "cannot be null"));
    }
    
    auto zone_id = zone->id();
    auto zone_name = zone->name();
    
    std::unique_lock lock(world_mutex_);
    zones_[zone_id] = std::move(zone);
    has_unsaved_changes_.store(true);
    
    auto logger = Log::game();
    logger->debug("Added zone: {} ({})", zone_name, zone_id);
    
    return Success();
}

void WorldManager::remove_zone(EntityId zone_id) {
    std::unique_lock lock(world_mutex_);
    
    auto it = zones_.find(zone_id);
    if (it != zones_.end()) {
        auto logger = Log::game();
        logger->debug("Removing zone: {} ({})", it->second->name(), zone_id);
        
        zones_.erase(it);
        has_unsaved_changes_.store(true);
    }
}

std::vector<std::shared_ptr<Zone>> WorldManager::get_all_zones() const {
    std::shared_lock lock(world_mutex_);
    std::vector<std::shared_ptr<Zone>> results;
    
    for (const auto& [id, zone] : zones_) {
        if (zone) {
            results.push_back(zone);
        }
    }
    
    return results;
}

std::shared_ptr<Zone> WorldManager::find_zone_by_name(std::string_view name) const {
    std::shared_lock lock(world_mutex_);
    
    for (const auto& [id, zone] : zones_) {
        if (zone && zone->name() == name) {
            return zone;
        }
    }
    
    return nullptr;
}

MovementResult WorldManager::move_actor(std::shared_ptr<Actor> actor, Direction direction) {
    if (!actor) {
        return MovementResult("Actor is null");
    }
    
    auto current_room = actor->current_room();
    if (!current_room) {
        return MovementResult("Actor not in valid room");
    }
    
    auto exit = current_room->get_exit(direction);
    if (!exit) {
        return MovementResult("No exit in that direction");
    }
    
    if (!exit->is_passable()) {
        return MovementResult("Exit is not passable");
    }
    
    auto destination_room = get_room(exit->to_room);
    if (!destination_room) {
        return MovementResult("Destination room does not exist");
    }
    
    auto movement_check = check_movement_restrictions(actor, current_room, destination_room, direction);
    if (!movement_check.success) {
        return movement_check;
    }
    
    // Execute the movement
    current_room->remove_actor(actor->id());
    destination_room->add_actor(actor);
    
    // Update actor's room
    actor->set_current_room(destination_room);
    
    // Notify callbacks
    notify_room_exit(actor, current_room);
    notify_room_enter(actor, destination_room);
    
    MovementResult result(true);
    result.from_room = current_room->id();
    result.to_room = destination_room->id();
    result.direction = direction;
    
    auto logger = Log::movement();
    logger->debug("Actor {} moved {} from {} to {}", 
                 actor->name(), direction, current_room->name(), destination_room->name());
    
    return result;
}

MovementResult WorldManager::move_actor_to_room(std::shared_ptr<Actor> actor, EntityId room_id) {
    if (!actor) {
        return MovementResult("Actor is null");
    }
    
    auto destination_room = get_room(room_id);
    if (!destination_room) {
        return MovementResult("Destination room does not exist");
    }
    
    auto current_room = actor->current_room();
    
    // Check movement restrictions
    auto movement_check = check_movement_restrictions(actor, current_room, destination_room, Direction::None);
    if (!movement_check.success) {
        return movement_check;
    }
    
    // Execute the movement
    if (current_room) {
        current_room->remove_actor(actor->id());
        notify_room_exit(actor, current_room);
    }
    
    destination_room->add_actor(actor);
    actor->set_current_room(destination_room);
    
    notify_room_enter(actor, destination_room);
    
    MovementResult result(true);
    result.from_room = current_room ? current_room->id() : INVALID_ENTITY_ID;
    result.to_room = destination_room->id();
    
    auto logger = Log::movement();
    logger->debug("Actor {} teleported to {}", actor->name(), destination_room->name());
    
    return result;
}

bool WorldManager::can_move_direction(std::shared_ptr<Actor> actor, Direction direction) const {
    if (!actor) {
        return false;
    }
    
    auto current_room = actor->current_room();
    if (!current_room) {
        return false;
    }
    
    auto exit = current_room->get_exit(direction);
    if (!exit || !exit->is_passable()) {
        return false;
    }
    
    auto destination_room = get_room(exit->to_room);
    if (!destination_room) {
        return false;
    }
    
    auto movement_check = check_movement_restrictions(actor, current_room, destination_room, direction);
    return movement_check.success;
}

std::shared_ptr<Room> WorldManager::get_room_in_direction(EntityId from_room, Direction direction) const {
    auto room = get_room(from_room);
    if (!room) {
        return nullptr;
    }
    
    auto exit = room->get_exit(direction);
    if (!exit) {
        return nullptr;
    }
    
    return get_room(exit->to_room);
}

Result<void> WorldManager::load_world_state() {
    // For now, delegate to load_world()
    return load_world();
}

void WorldManager::process_zone_resets() {
    std::shared_lock lock(world_mutex_);
    auto now = std::chrono::steady_clock::now();
    
    // Check scheduled resets
    for (auto it = scheduled_resets_.begin(); it != scheduled_resets_.end();) {
        if (now >= it->second) {
            EntityId zone_id = it->first;
            it = scheduled_resets_.erase(it);
            
            auto zone = get_zone(zone_id);
            if (zone) {
                zone->force_reset();
                notify_zone_reset(zone);
            }
        } else {
            ++it;
        }
    }
    
    // Check natural zone resets
    for (const auto& [id, zone] : zones_) {
        if (zone && zone->needs_reset()) {
            zone->force_reset();
            notify_zone_reset(zone);
        }
    }
}

void WorldManager::force_zone_reset(EntityId zone_id) {
    auto zone = get_zone(zone_id);
    if (zone) {
        zone->force_reset();
        notify_zone_reset(zone);
    }
}

void WorldManager::schedule_zone_reset(EntityId zone_id, std::chrono::seconds delay) {
    auto reset_time = std::chrono::steady_clock::now() + delay;
    
    std::unique_lock lock(world_mutex_);
    scheduled_resets_[zone_id] = reset_time;
    
    auto logger = Log::game();
    logger->debug("Scheduled zone {} reset in {}s", zone_id, delay.count());
}

ValidationResult WorldManager::validate_world() const {
    ValidationResult result;
    
    std::shared_lock lock(world_mutex_);
    
    // Validate zones
    for (const auto& [id, zone] : zones_) {
        if (zone) {
            validate_zone_integrity(zone, result);
        }
    }
    
    // Validate rooms
    for (const auto& [id, room] : rooms_) {
        if (room) {
            validate_room_exits(room, result);
            
            // Check if room belongs to a zone
            bool found_in_zone = false;
            for (const auto& [zone_id, zone] : zones_) {
                if (zone && zone->contains_room(room->id())) {
                    found_in_zone = true;
                    break;
                }
            }
            
            if (!found_in_zone) {
                result.orphaned_rooms++;
                result.add_warning(fmt::format("Room {} ({}) is not in any zone", 
                                              room->name(), room->id()));
            }
        }
    }
    
    // TODO: Check for unreachable rooms using pathfinding from start room
    
    return result;
}

Result<void> WorldManager::repair_world_issues() {
    auto validation = validate_world();
    
    if (validation.is_valid) {
        return Success();
    }
    
    auto logger = Log::game();
    logger->info("Attempting to repair {} world issues", validation.errors.size());
    
    // TODO: Implement automatic world repair
    // For now, just log the issues
    for (const auto& error : validation.errors) {
        logger->error("World repair needed: {}", error);
    }
    
    return std::unexpected(Errors::NotImplemented("World repair"));
}

void WorldManager::update_statistics() {
    std::shared_lock lock(world_mutex_);
    
    stats_.zones_loaded = static_cast<int>(zones_.size());
    stats_.rooms_loaded = static_cast<int>(rooms_.size());
    
    // TODO: Count objects and mobiles
    stats_.objects_loaded = 0;
    stats_.mobiles_loaded = 0;
}

std::vector<std::shared_ptr<Room>> WorldManager::find_rooms_with_flag(RoomFlag flag) const {
    std::shared_lock lock(world_mutex_);
    std::vector<std::shared_ptr<Room>> results;
    
    for (const auto& [id, room] : rooms_) {
        if (room && room->has_flag(flag)) {
            results.push_back(room);
        }
    }
    
    return results;
}

std::vector<std::shared_ptr<Zone>> WorldManager::find_zones_with_flag(ZoneFlag flag) const {
    std::shared_lock lock(world_mutex_);
    std::vector<std::shared_ptr<Zone>> results;
    
    for (const auto& [id, zone] : zones_) {
        if (zone && zone->has_flag(flag)) {
            results.push_back(zone);
        }
    }
    
    return results;
}

std::vector<std::shared_ptr<Room>> WorldManager::find_rooms_by_sector(SectorType sector) const {
    std::shared_lock lock(world_mutex_);
    std::vector<std::shared_ptr<Room>> results;
    
    for (const auto& [id, room] : rooms_) {
        if (room && room->sector_type() == sector) {
            results.push_back(room);
        }
    }
    
    return results;
}

std::vector<std::shared_ptr<Room>> WorldManager::find_rooms_in_level_range(int min_level, int max_level) const {
    std::shared_lock lock(world_mutex_);
    std::vector<std::shared_ptr<Room>> results;
    
    for (const auto& [id, room] : rooms_) {
        if (room) {
            auto zone = get_zone(room->zone_id());
            if (zone && zone->allows_level(min_level) && zone->allows_level(max_level)) {
                results.push_back(room);
            }
        }
    }
    
    return results;
}

std::vector<Direction> WorldManager::find_path(EntityId from_room, EntityId to_room, 
                                              std::shared_ptr<Actor> actor) const {
    // Simple breadth-first search pathfinding
    std::queue<EntityId> queue;
    std::unordered_map<EntityId, EntityId> came_from;
    std::unordered_map<EntityId, Direction> directions;
    std::unordered_set<EntityId> visited;
    
    queue.push(from_room);
    visited.insert(from_room);
    
    while (!queue.empty()) {
        EntityId current = queue.front();
        queue.pop();
        
        if (current == to_room) {
            return reconstruct_path(came_from, directions, from_room, to_room);
        }
        
        auto room = get_room(current);
        if (!room) {
            continue;
        }
        
        auto available_exits = room->get_available_exits();
        for (Direction dir : available_exits) {
            auto exit = room->get_exit(dir);
            if (!exit || !exit->is_passable()) {
                continue;
            }
            
            EntityId next_room = exit->to_room;
            if (visited.contains(next_room)) {
                continue;
            }
            
            auto next_room_ptr = get_room(next_room);
            if (!next_room_ptr || !is_passable_for_actor(next_room_ptr, actor)) {
                continue;
            }
            
            visited.insert(next_room);
            came_from[next_room] = current;
            directions[next_room] = dir;
            queue.push(next_room);
        }
    }
    
    return {}; // No path found
}

int WorldManager::calculate_distance(EntityId from_room, EntityId to_room) const {
    if (from_room == to_room) {
        return 0;
    }
    
    auto path = find_path(from_room, to_room);
    return static_cast<int>(path.size());
}

void WorldManager::enable_file_watching(bool enabled) {
    file_watching_enabled_.store(enabled);
    
    if (enabled) {
        update_file_timestamps();
    }
    
    auto logger = Log::game();
    logger->info("File watching {}", enabled ? "enabled" : "disabled");
}

void WorldManager::check_for_file_changes() {
    if (!file_watching_enabled_.load()) {
        return;
    }
    
    auto changed_files = get_changed_files();
    if (changed_files.empty()) {
        return;
    }
    
    auto logger = Log::game();
    logger->info("Detected {} changed world files", changed_files.size());
    
    for (const auto& filename : changed_files) {
        logger->debug("File changed: {}", filename);
        
        // Try to reload the file
        if (filename.ends_with(".json")) {
            auto result = load_zone_file(filename);
            if (!result) {
                logger->error("Failed to reload zone file {}: {}", filename, result.error().message);
            }
        }
        // TODO: Handle room file changes
    }
    
    update_file_timestamps();
}

std::string WorldManager::get_world_summary() const {
    std::shared_lock lock(world_mutex_);
    
    return fmt::format(
        "World Summary:\n"
        "  Zones: {}\n"
        "  Rooms: {}\n"
        "  Objects: {}\n"
        "  Mobiles: {}\n"
        "  Uptime: {}s\n"
        "  Last Save: {}s ago\n"
        "  Load Time: {}ms\n"
        "  Save Time: {}ms",
        zones_.size(),
        rooms_.size(),
        stats_.objects_loaded,
        stats_.mobiles_loaded,
        stats_.uptime().count(),
        stats_.time_since_save().count(),
        stats_.load_time.count(),
        stats_.save_time.count()
    );
}

std::vector<std::string> WorldManager::get_zone_list() const {
    std::shared_lock lock(world_mutex_);
    std::vector<std::string> zone_list;
    
    for (const auto& [id, zone] : zones_) {
        if (zone) {
            zone_list.push_back(fmt::format("{}: {}", id, zone->name()));
        }
    }
    
    std::sort(zone_list.begin(), zone_list.end());
    return zone_list;
}

std::vector<std::string> WorldManager::get_orphaned_rooms() const {
    std::shared_lock lock(world_mutex_);
    std::vector<std::string> orphaned;
    
    for (const auto& [id, room] : rooms_) {
        if (!room) continue;
        
        bool found_in_zone = false;
        for (const auto& [zone_id, zone] : zones_) {
            if (zone && zone->contains_room(room->id())) {
                found_in_zone = true;
                break;
            }
        }
        
        if (!found_in_zone) {
            orphaned.push_back(fmt::format("{}: {}", id, room->name()));
        }
    }
    
    return orphaned;
}

void WorldManager::dump_world_state(const std::string& filename) const {
    std::shared_lock lock(world_mutex_);
    
    nlohmann::json dump;
    
    // Dump zones
    nlohmann::json zones_json = nlohmann::json::array();
    for (const auto& [id, zone] : zones_) {
        if (zone) {
            zones_json.push_back(zone->to_json());
        }
    }
    dump["zones"] = zones_json;
    
    // Dump rooms
    nlohmann::json rooms_json = nlohmann::json::array();
    for (const auto& [id, room] : rooms_) {
        if (room) {
            rooms_json.push_back(room->to_json());
        }
    }
    dump["rooms"] = rooms_json;
    
    // Dump statistics
    dump["statistics"] = {
        {"zones_loaded", stats_.zones_loaded},
        {"rooms_loaded", stats_.rooms_loaded},
        {"objects_loaded", stats_.objects_loaded},
        {"mobiles_loaded", stats_.mobiles_loaded},
        {"uptime_seconds", stats_.uptime().count()},
        {"load_time_ms", stats_.load_time.count()},
        {"save_time_ms", stats_.save_time.count()}
    };
    
    std::ofstream file(filename);
    if (file.is_open()) {
        file << dump.dump(2);
    }
}

Result<void> WorldManager::import_world_state(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        return std::unexpected(Errors::FileNotFound(filename));
    }
    
    nlohmann::json dump;
    try {
        file >> dump;
    } catch (const nlohmann::json::exception& e) {
        return std::unexpected(Errors::ParseError("World state import", e.what()));
    }
    
    std::unique_lock lock(world_mutex_);
    
    // Clear existing data
    rooms_.clear();
    zones_.clear();
    
    // Import zones
    if (dump.contains("zones") && dump["zones"].is_array()) {
        for (const auto& zone_json : dump["zones"]) {
            auto zone_result = Zone::from_json(zone_json);
            if (zone_result) {
                auto zone = std::move(zone_result.value());
                zones_[zone->id()] = std::move(zone);
            }
        }
    }
    
    // Import rooms
    if (dump.contains("rooms") && dump["rooms"].is_array()) {
        for (const auto& room_json : dump["rooms"]) {
            auto room_result = Room::from_json(room_json);
            if (room_result) {
                auto room = std::move(room_result.value());
                rooms_[room->id()] = std::move(room);
            }
        }
    }
    
    has_unsaved_changes_.store(true);
    
    auto logger = Log::game();
    logger->info("Imported world state from: {}", filename);
    
    return Success();
}

// Private helper methods

Result<void> WorldManager::load_zones_from_directory(const std::string& world_dir) {
    if (!std::filesystem::exists(world_dir)) {
        return std::unexpected(Errors::FileNotFound(world_dir));
    }
    
    auto zone_files = WorldUtils::get_zone_files(world_dir);
    
    for (const auto& filename : zone_files) {
        std::string full_path = world_dir + "/" + filename;
        auto result = load_zone_file(full_path);
        if (!result) {
            auto logger = Log::game();
            logger->error("Failed to load zone file {}: {}", filename, result.error().message);
            stats_.failed_zone_loads++;
        }
    }
    
    return Success();
}

Result<void> WorldManager::load_rooms_from_directory(const std::string& room_dir) {
    // For now, rooms are loaded as part of zone files
    // TODO: Implement separate room file loading if needed
    return Success();
}

void WorldManager::validate_room_exits(std::shared_ptr<Room> room, ValidationResult& result) const {
    auto exits = room->get_available_exits();
    
    for (Direction dir : exits) {
        auto exit = room->get_exit(dir);
        if (!exit) continue;
        
        if (!exit->to_room.is_valid()) {
            result.missing_exits++;
            result.add_error(fmt::format("Room {} has invalid exit {} destination", 
                                       room->id(), dir));
            continue;
        }
        
        if (exit->to_room == room->id()) {
            result.circular_exits++;
            result.add_warning(fmt::format("Room {} has circular exit {}", 
                                         room->id(), dir));
            continue;
        }
        
        if (!get_room(exit->to_room)) {
            result.missing_exits++;
            result.add_error(fmt::format("Room {} exit {} points to non-existent room {}", 
                                       room->id(), dir, exit->to_room));
        }
    }
}

void WorldManager::validate_zone_integrity(std::shared_ptr<Zone> zone, ValidationResult& result) const {
    auto zone_result = zone->validate();
    if (!zone_result) {
        result.add_error(fmt::format("Zone {} validation failed: {}", 
                                   zone->id(), zone_result.error().message));
    }
    
    // Check if zone rooms exist
    for (EntityId room_id : zone->rooms()) {
        if (!get_room(room_id)) {
            result.add_error(fmt::format("Zone {} references non-existent room {}", 
                                       zone->id(), room_id));
        }
    }
}

MovementResult WorldManager::check_movement_restrictions(std::shared_ptr<Actor> actor,
                                                        std::shared_ptr<Room> from_room,
                                                        std::shared_ptr<Room> to_room,
                                                        Direction direction) const {
    if (!to_room->can_accommodate(actor.get())) {
        return MovementResult("Destination room is full");
    }
    
    // TODO: Check level restrictions, PK flags, etc.
    
    return MovementResult(true);
}

void WorldManager::notify_room_enter(std::shared_ptr<Actor> actor, std::shared_ptr<Room> room) {
    if (room_enter_callback_) {
        room_enter_callback_(actor, room);
    }
}

void WorldManager::notify_room_exit(std::shared_ptr<Actor> actor, std::shared_ptr<Room> room) {
    if (room_exit_callback_) {
        room_exit_callback_(actor, room);
    }
}

void WorldManager::notify_zone_reset(std::shared_ptr<Zone> zone) {
    if (zone_reset_callback_) {
        zone_reset_callback_(zone);
    }
}

void WorldManager::notify_object_create(std::shared_ptr<Object> object) {
    if (object_create_callback_) {
        object_create_callback_(object);
    }
}

void WorldManager::update_file_timestamps() {
    file_timestamps_.clear();
    
    if (std::filesystem::exists(world_path_)) {
        for (const auto& entry : std::filesystem::directory_iterator(world_path_)) {
            if (entry.is_regular_file() && entry.path().extension() == ".json") {
                file_timestamps_[entry.path().string()] = entry.last_write_time();
            }
        }
    }
}

std::vector<std::string> WorldManager::get_changed_files() {
    std::vector<std::string> changed_files;
    
    if (!std::filesystem::exists(world_path_)) {
        return changed_files;
    }
    
    for (const auto& entry : std::filesystem::directory_iterator(world_path_)) {
        if (!entry.is_regular_file() || entry.path().extension() != ".json") {
            continue;
        }
        
        std::string filepath = entry.path().string();
        auto current_time = entry.last_write_time();
        
        auto it = file_timestamps_.find(filepath);
        if (it == file_timestamps_.end() || it->second != current_time) {
            changed_files.push_back(filepath);
        }
    }
    
    return changed_files;
}

std::vector<Direction> WorldManager::reconstruct_path(
    const std::unordered_map<EntityId, EntityId>& came_from,
    const std::unordered_map<EntityId, Direction>& directions,
    EntityId start, EntityId goal) const {
    
    std::vector<Direction> path;
    EntityId current = goal;
    
    while (current != start) {
        auto dir_it = directions.find(current);
        if (dir_it == directions.end()) {
            break;
        }
        
        path.push_back(dir_it->second);
        
        auto came_it = came_from.find(current);
        if (came_it == came_from.end()) {
            break;
        }
        
        current = came_it->second;
    }
    
    std::reverse(path.begin(), path.end());
    return path;
}

bool WorldManager::is_passable_for_actor(std::shared_ptr<Room> room, std::shared_ptr<Actor> actor) const {
    if (!room) {
        return false;
    }
    
    if (!room->can_accommodate(actor.get())) {
        return false;
    }
    
    // TODO: Check actor-specific restrictions (level, alignment, etc.)
    
    return true;
}

int WorldManager::get_movement_cost(std::shared_ptr<Room> from_room, std::shared_ptr<Room> to_room,
                                   std::shared_ptr<Actor> actor) const {
    if (!to_room) {
        return 1000; // Very high cost for invalid rooms
    }
    
    int base_cost = RoomUtils::get_movement_cost(to_room->sector_type());
    
    // TODO: Apply actor-specific modifiers
    
    return base_cost;
}

Result<void> WorldManager::spawn_mobile_in_zone(Mobile* prototype, EntityId zone_id) {
    if (!prototype) {
        return std::unexpected(Errors::InvalidArgument("prototype", "cannot be null"));
    }
    
    auto zone = get_zone(zone_id);
    if (!zone) {
        return std::unexpected(Errors::NotFound("zone"));
    }
    
    auto logger = Log::game();
    
    // Create a new mobile instance from the prototype
    auto new_mobile_result = Mobile::create(prototype->id(), prototype->name(), prototype->stats().level);
    if (!new_mobile_result) {
        return std::unexpected(new_mobile_result.error());
    }
    
    auto new_mobile = std::move(new_mobile_result.value());
    
    // Copy stats and properties from prototype
    new_mobile->stats() = prototype->stats();
    new_mobile->set_aggressive(prototype->is_aggressive());
    new_mobile->set_aggression_level(prototype->aggression_level());
    
    // Find appropriate room for spawning - for now, spawn in the first room of the zone
    auto zone_rooms = get_rooms_in_zone(zone_id);
    if (zone_rooms.empty()) {
        logger->warn("No rooms available in zone {} for spawning mobile {}", zone_id, prototype->name());
        return std::unexpected(Errors::InvalidState("No rooms in zone"));
    }
    
    // For the test zone, spawn specific mobs in specific rooms
    std::shared_ptr<Room> spawn_room = nullptr;
    
    if (prototype->name() == "a practice dummy") {
        // Spawn practice dummy in training room (102)
        spawn_room = get_room(EntityId{102});
    } else if (prototype->name() == "a training guard") {
        // Spawn training guard in starting room (100)
        spawn_room = get_room(EntityId{100});
    }
    
    // Fallback to first room if specific placement didn't work
    if (!spawn_room) {
        spawn_room = zone_rooms[0];
    }
    
    // Place the mobile in the room
    auto actor_ptr = std::shared_ptr<Actor>(new_mobile.release());
    auto move_result = move_actor_to_room(actor_ptr, spawn_room->id());
    if (!move_result.success) {
        logger->error("Failed to place mobile {} in room {}: {}", 
                     prototype->name(), spawn_room->id(), move_result.failure_reason);
        return std::unexpected(Errors::InvalidState(move_result.failure_reason));
    }
    
    logger->info("Spawned mobile '{}' in room '{}' ({})", 
                prototype->name(), spawn_room->name(), spawn_room->id());
    
    return Success();
}

void WorldManager::setup_zone_callbacks(std::shared_ptr<Zone> zone) {
    if (!zone) return;
    
    // Set up mobile spawning callback
    zone->set_spawn_mobile_callback([this](EntityId mobile_id, EntityId room_id) -> std::shared_ptr<Mobile> {
        return spawn_mobile_for_zone(mobile_id, room_id);
    });
    
    // Set up object spawning callback
    zone->set_spawn_object_callback([this](EntityId object_id, EntityId room_id) -> std::shared_ptr<Object> {
        return spawn_object_for_zone(object_id, room_id);
    });
    
    // Set up room lookup callback
    zone->set_get_room_callback([this](EntityId room_id) -> std::shared_ptr<Room> {
        return get_room(room_id);
    });
    
    // Set up zone mobile cleanup callback
    zone->set_cleanup_zone_mobiles_callback([this](EntityId zone_id) {
        cleanup_zone_mobiles(zone_id);
    });
}

std::shared_ptr<Mobile> WorldManager::spawn_mobile_for_zone(EntityId mobile_id, EntityId room_id) {
    auto logger = Log::game();
    
    // Find mobile prototype
    auto mobile_it = mobiles_.find(mobile_id);
    if (mobile_it == mobiles_.end()) {
        logger->error("Cannot spawn mobile {} - prototype not found", mobile_id);
        return nullptr;
    }
    
    Mobile* prototype = mobile_it->second;
    
    // Create new mobile instance from prototype
    auto new_mobile_result = Mobile::create(prototype->id(), prototype->name(), prototype->stats().level);
    if (!new_mobile_result) {
        logger->error("Failed to create mobile instance: {}", new_mobile_result.error().message);
        return nullptr;
    }
    
    auto new_mobile = std::move(new_mobile_result.value());
    
    // Copy stats and properties from prototype
    new_mobile->set_keywords(prototype->keywords());
    new_mobile->set_short_description(prototype->short_description());
    new_mobile->set_description(prototype->description());
    new_mobile->stats() = prototype->stats();
    new_mobile->set_aggressive(prototype->is_aggressive());
    new_mobile->set_aggression_level(prototype->aggression_level());
    
    // Place the mobile in the specified room
    auto actor_ptr = std::shared_ptr<Actor>(new_mobile.release());
    auto move_result = move_actor_to_room(actor_ptr, room_id);
    if (!move_result.success) {
        logger->error("Failed to place spawned mobile in room {}: {}", 
                     room_id, move_result.failure_reason);
        return nullptr;
    }
    
    auto mobile_ptr = std::static_pointer_cast<Mobile>(actor_ptr);
    
    // Register the spawned mobile for efficient lookups
    register_spawned_mobile(mobile_ptr);
    
    return mobile_ptr;
}

std::shared_ptr<Object> WorldManager::spawn_object_for_zone(EntityId object_id, EntityId room_id) {
    auto logger = Log::game();
    
    // Find object prototype
    auto object_it = objects_.find(object_id);
    if (object_it == objects_.end()) {
        logger->error("Cannot spawn object {} - prototype not found", object_id);
        return nullptr;
    }
    
    Object* prototype = object_it->second;
    
    // Create new object instance from prototype
    auto new_object_result = Object::create(prototype->id(), prototype->name(), prototype->type());
    if (!new_object_result) {
        logger->error("Failed to create object instance: {}", new_object_result.error().message);
        return nullptr;
    }
    
    auto new_object = std::move(new_object_result.value());
    
    // Copy properties from prototype
    new_object->set_short_description(prototype->short_description());
    new_object->set_description(prototype->description());
    new_object->set_weight(prototype->weight());
    new_object->set_value(prototype->value());
    new_object->set_equip_slot(prototype->equip_slot());
    new_object->set_armor_class(prototype->armor_class());
    new_object->set_damage_profile(prototype->damage_profile());
    new_object->set_container_info(prototype->container_info());
    new_object->set_light_info(prototype->light_info());
    
    // Check if this is an encoded equipment request (mobile_id | (equipment_slot << 32))
    uint64_t room_id_value = room_id.value();
    uint64_t potential_mobile_id = room_id_value & 0xFFFFFFFF; // Lower 32 bits
    uint64_t potential_equipment_slot = room_id_value >> 32;   // Upper 32 bits
    
    // If upper 32 bits contain data, this is likely an equipment request
    if (potential_equipment_slot > 0 && potential_equipment_slot < 32) {
        // This looks like an equipment request - find the mobile and equip the item
        EntityId mobile_id{potential_mobile_id};
        
        // Use efficient O(1) lookup for the mobile
        std::shared_ptr<Mobile> target_mobile = find_spawned_mobile(mobile_id);
        
        if (!target_mobile) {
            logger->error("Cannot equip object {} - mobile {} not found", object_id, mobile_id);
            return nullptr;
        }
        
        // Convert equipment slot number to EquipSlot enum
        EquipSlot equip_slot = static_cast<EquipSlot>(potential_equipment_slot);
        
        auto shared_object = std::shared_ptr<Object>(new_object.release());
        
        // Try to equip the item on the mobile
        auto equip_result = target_mobile->equipment().equip_item(shared_object);
        if (!equip_result) {
            logger->warn("Failed to equip object {} on mobile {}: {}", 
                        object_id, mobile_id, equip_result.error().message);
            // If equip fails, add to inventory instead
            auto give_result = target_mobile->give_item(shared_object);
            if (!give_result) {
                logger->error("Failed to give object {} to mobile {}: {}", 
                            object_id, mobile_id, give_result.error().message);
                return nullptr;
            }
        }
        
        logger->debug("Successfully equipped object {} on mobile {} in slot {}", 
                     object_id, mobile_id, potential_equipment_slot);
        return shared_object;
    }
    
    // Standard room placement
    auto room = get_room(room_id);
    if (!room) {
        logger->error("Cannot place object {} - room {} not found", object_id, room_id);
        return nullptr;
    }
    
    auto shared_object = std::shared_ptr<Object>(new_object.release());
    room->add_object(shared_object);
    
    return shared_object;
}

Result<void> WorldManager::spawn_mobile_in_specific_room(Mobile* prototype, EntityId room_id) {
    if (!prototype) {
        return std::unexpected(Errors::InvalidArgument("prototype", "cannot be null"));
    }
    
    auto logger = Log::game();
    
    // Create a new mobile instance from the prototype
    auto new_mobile_result = Mobile::create(prototype->id(), prototype->name(), prototype->stats().level);
    if (!new_mobile_result) {
        return std::unexpected(new_mobile_result.error());
    }
    
    auto new_mobile = std::move(new_mobile_result.value());
    
    // Copy stats and properties from prototype
    new_mobile->stats() = prototype->stats();
    new_mobile->set_aggressive(prototype->is_aggressive());
    new_mobile->set_aggression_level(prototype->aggression_level());
    
    // Place the mobile in the specified room
    auto spawn_room = get_room(room_id);
    if (!spawn_room) {
        logger->error("Cannot spawn mobile {} - room {} not found", prototype->name(), room_id);
        return std::unexpected(Errors::NotFound("room"));
    }
    
    auto actor_ptr = std::shared_ptr<Actor>(new_mobile.release());
    auto move_result = move_actor_to_room(actor_ptr, room_id);
    if (!move_result.success) {
        logger->error("Failed to place mobile {} in room {}: {}", 
                     prototype->name(), spawn_room->id(), move_result.failure_reason);
        return std::unexpected(Errors::InvalidState(move_result.failure_reason));
    }
    
    // Register the spawned mobile for efficient lookups
    auto mobile_ptr = std::static_pointer_cast<Mobile>(actor_ptr);
    register_spawned_mobile(mobile_ptr);
    
    logger->info("Spawned mobile '{}' in room '{}' ({})", 
                prototype->name(), spawn_room->name(), spawn_room->id());
    
    return Success();
}

// Mobile instance tracking for efficient lookups
void WorldManager::register_spawned_mobile(std::shared_ptr<Mobile> mobile) {
    if (!mobile) return;
    
    std::lock_guard<std::shared_mutex> lock(world_mutex_);
    spawned_mobiles_[mobile->id()] = mobile;
    
    auto logger = Log::game();
    logger->debug("Registered spawned mobile {} ({})", mobile->name(), mobile->id());
}

void WorldManager::unregister_spawned_mobile(EntityId mobile_id) {
    std::lock_guard<std::shared_mutex> lock(world_mutex_);
    auto it = spawned_mobiles_.find(mobile_id);
    if (it != spawned_mobiles_.end()) {
        auto logger = Log::game();
        logger->debug("Unregistered spawned mobile {} ({})", it->second->name(), mobile_id);
        spawned_mobiles_.erase(it);
    }
}

std::shared_ptr<Mobile> WorldManager::find_spawned_mobile(EntityId mobile_id) const {
    std::shared_lock<std::shared_mutex> lock(world_mutex_);
    auto it = spawned_mobiles_.find(mobile_id);
    return (it != spawned_mobiles_.end()) ? it->second : nullptr;
}

void WorldManager::cleanup_zone_mobiles(EntityId zone_id) {
    auto logger = Log::game();
    logger->debug("Cleaning up mobiles in zone {}", zone_id);
    
    auto zone_it = zones_.find(zone_id);
    if (zone_it == zones_.end()) {
        logger->warn("Zone {} not found during mobile cleanup", zone_id);
        return;
    }
    
    auto zone = zone_it->second;
    std::vector<EntityId> mobiles_to_remove;
    
    // Find all mobiles in rooms belonging to this zone
    for (EntityId room_id : zone->rooms()) {
        auto room_it = rooms_.find(room_id);
        if (room_it == rooms_.end()) {
            continue; // Room not found, skip
        }
        
        auto room = room_it->second;
        // Copy actor IDs since we'll be modifying the collection
        auto actors = room->contents().actors;  
        
        for (const auto& actor : actors) {
            // Check if this actor is a mobile (not a player)
            auto mobile = std::dynamic_pointer_cast<Mobile>(actor);
            if (mobile) {
                mobiles_to_remove.push_back(mobile->id());
                // Remove mobile from room
                room->remove_actor(mobile->id());
                logger->debug("Removed mobile {} ({}) from room {}", mobile->name(), mobile->id(), room_id);
            }
        }
    }
    
    // Unregister mobiles from tracking system
    {
        std::lock_guard<std::shared_mutex> lock(world_mutex_);
        for (EntityId mobile_id : mobiles_to_remove) {
            auto it = spawned_mobiles_.find(mobile_id);
            if (it != spawned_mobiles_.end()) {
                logger->debug("Unregistered spawned mobile {} from tracking", mobile_id);
                spawned_mobiles_.erase(it);
            }
        }
    }
    
    if (!mobiles_to_remove.empty()) {
        logger->info("Cleaned up {} mobiles from zone {}", mobiles_to_remove.size(), zone_id);
    }
}

// Weather Integration Implementation

void WorldManager::update_weather_system(std::chrono::minutes elapsed) {
    // Update global weather system
    Weather().update_weather(elapsed);
    Weather().process_weather_effects();
    
    // Apply weather effects to zones
    for (const auto& [zone_id, zone] : zones_) {
        if (!zone || zone->has_weather_override()) {
            continue; // Skip zones that override weather
        }
        
        auto weather_effects = Weather().get_weather_effects(zone_id);
        
        // Apply weather effects to zone statistics and properties
        // This could include adjusting spawn rates, visibility, etc.
        // For now, we just ensure weather state is tracked
    }
}

void WorldManager::initialize_weather_callbacks() {
    auto logger = Log::game();
    
    // Initialize weather system
    auto weather_result = WeatherSystem::initialize();
    if (!weather_result) {
        logger->error("Failed to initialize weather system: {}", weather_result.error().message);
        return;
    }
    
    // Set up weather change callback for logging and zone effects
    Weather().set_weather_change_callback([logger](EntityId zone_id, const WeatherState& old_state, const WeatherState& new_state) {
        if (zone_id == INVALID_ENTITY_ID) {
            logger->info("Global weather changed from {} to {}", old_state.get_summary(), new_state.get_summary());
        } else {
            logger->info("Zone {} weather changed from {} to {}", zone_id, old_state.get_summary(), new_state.get_summary());
        }
    });
    
    // Initialize weather configs for loaded zones
    for (const auto& [zone_id, zone] : zones_) {
        if (!zone) continue;
        
        WeatherConfig config;
        
        // Set weather pattern based on zone characteristics
        if (zone->is_underground()) {
            config.pattern = WeatherPattern::Stable;
            config.override_global = true;
            Weather().set_zone_weather(zone_id, WeatherType::Clear, WeatherIntensity::Calm);
        } else if (zone->has_flag(ZoneFlag::Astral)) {
            config.pattern = WeatherPattern::Magical;
            config.override_global = true;
        } else if (zone->is_quest_zone()) {
            config.pattern = WeatherPattern::Variable;
        }
        
        Weather().set_zone_weather_config(zone_id, config);
    }
    
    logger->info("Weather system integration initialized");
}

// WorldUtils Implementation

namespace WorldUtils {
    bool validate_world_directory(const std::string& world_path) {
        return std::filesystem::exists(world_path);
    }
    
    Result<void> create_world_directory(const std::string& world_path) {
        try {
            std::filesystem::create_directories(world_path);
            
            return Success();
        } catch (const std::filesystem::filesystem_error& e) {
            return std::unexpected(Errors::FileAccessError(e.what()));
        }
    }
    
    std::vector<std::string> get_zone_files(const std::string& world_dir) {
        std::vector<std::string> zone_files;
        
        if (!std::filesystem::exists(world_dir)) {
            return zone_files;
        }
        
        for (const auto& entry : std::filesystem::directory_iterator(world_dir)) {
            if (entry.is_regular_file() && entry.path().extension() == ".json") {
                zone_files.push_back(entry.path().filename().string());
            }
        }
        
        std::sort(zone_files.begin(), zone_files.end());
        return zone_files;
    }
    
    std::vector<std::string> get_room_files(const std::string& room_dir) {
        std::vector<std::string> room_files;
        
        if (!std::filesystem::exists(room_dir)) {
            return room_files;
        }
        
        for (const auto& entry : std::filesystem::directory_iterator(room_dir)) {
            if (entry.is_regular_file() && entry.path().extension() == ".wld") {
                room_files.push_back(entry.path().filename().string());
            }
        }
        
        std::sort(room_files.begin(), room_files.end());
        return room_files;
    }
    
    Result<void> backup_world_data(const std::string& world_path, const std::string& backup_path) {
        try {
            if (std::filesystem::exists(backup_path)) {
                std::filesystem::remove_all(backup_path);
            }
            
            std::filesystem::copy(world_path, backup_path, 
                                std::filesystem::copy_options::recursive);
            
            return Success();
        } catch (const std::filesystem::filesystem_error& e) {
            return std::unexpected(Errors::FileAccessError(e.what()));
        }
    }
    
    Result<void> restore_world_data(const std::string& backup_path, const std::string& world_path) {
        try {
            if (std::filesystem::exists(world_path)) {
                std::filesystem::remove_all(world_path);
            }
            
            std::filesystem::copy(backup_path, world_path,
                                std::filesystem::copy_options::recursive);
            
            return Success();
        } catch (const std::filesystem::filesystem_error& e) {
            return std::unexpected(Errors::FileAccessError(e.what()));
        }
    }
    
    std::unordered_map<std::string, int> calculate_world_metrics(const WorldManager& world) {
        std::unordered_map<std::string, int> metrics;
        
        metrics["total_zones"] = static_cast<int>(world.zone_count());
        metrics["total_rooms"] = static_cast<int>(world.room_count());
        
        // Count rooms by sector type
        for (int i = 0; i < magic_enum::enum_count<SectorType>(); ++i) {
            auto sector = static_cast<SectorType>(i);
            auto rooms = world.find_rooms_by_sector(sector);
            std::string key = fmt::format("rooms_{}", magic_enum::enum_name(sector));
            metrics[key] = static_cast<int>(rooms.size());
        }
        
        // Count zones by flag
        for (int i = 0; i < magic_enum::enum_count<ZoneFlag>(); ++i) {
            auto flag = static_cast<ZoneFlag>(i);
            auto zones = world.find_zones_with_flag(flag);
            std::string key = fmt::format("zones_{}", magic_enum::enum_name(flag));
            metrics[key] = static_cast<int>(zones.size());
        }
        
        return metrics;
    }
    
    Result<void> export_world(const WorldManager& world, const std::string& filename, 
                             std::string_view format) {
        if (format == "json") {
            world.dump_world_state(filename);
            return Success();
        }
        
        return std::unexpected(Errors::NotImplemented(fmt::format("Export format: {}", format)));
    }
    
    Result<void> import_world(WorldManager& world, const std::string& filename,
                             std::string_view format) {
        if (format == "json") {
            return world.import_world_state(filename);
        }
        
        return std::unexpected(Errors::NotImplemented(fmt::format("Import format: {}", format)));
    }
}