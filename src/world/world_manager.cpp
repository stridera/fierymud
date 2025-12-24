#include "world_manager.hpp"
#include "weather.hpp"
#include "../core/actor.hpp"
#include "../core/object.hpp"
#include "../core/logging.hpp"
#include "../core/shopkeeper.hpp"
#include "../database/connection_pool.hpp"
#include "../database/game_data_cache.hpp"
#include "../database/world_queries.hpp"

#include <filesystem>
#include <fstream>
#include <algorithm>
#include <queue>
#include <unordered_set>

// World manager constants
namespace {
    // Zone ID remapping (zone 0 conflicts with modern system)
    constexpr uint64_t REMAPPED_ZONE_ZERO_ID = 1000;

    // Pathfinding constants
    constexpr int INVALID_ROOM_MOVEMENT_COST = 1000;  // Very high cost for unreachable rooms
    constexpr int FLYING_OPTIMAL_COST = 1;            // Minimal cost for flying in air

    // Test zone room IDs
    constexpr uint64_t TEST_STARTING_ROOM_ID = 100;
    constexpr uint64_t TEST_TRAINING_ROOM_ID = 102;

    // Mobile inventory constraints
    constexpr int MOBILE_INVENTORY_MAX_WEIGHT = 100;
}

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
    last_spawned_objects_.clear();  // Clear object instance tracking for containers
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

    // Load game data cache (classes, races) before world data
    auto cache_result = GameDataCache::instance().load_all();
    if (!cache_result) {
        logger->warn("Failed to load game data cache: {}", cache_result.error().message);
        // Continue loading - the cache will use fallback values
    }

    // Load zones and rooms from PostgreSQL database
    TRY(load_zones_from_database());
    
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
                // Convert 0 to a high number since the modern system doesn't like ID 0
                if (id_value == 0) {
                    id_value = REMAPPED_ZONE_ZERO_ID;
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
    
    logger->debug("Loaded zone: {} ({})", zone_name, zone_id);
    
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
    if (it != rooms_.end()) {
        return it->second;
    }
    return nullptr;
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
                 actor->display_name(), direction, current_room->display_name(), destination_room->display_name());
    
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
    logger->debug("Actor {} teleported to {}", 
                 actor->display_name(), destination_room->display_name());
    
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

    // Check for unreachable rooms using pathfinding from start room
    if (start_room_.is_valid() && get_room(start_room_)) {
        std::unordered_set<EntityId> reachable;
        std::queue<EntityId> to_visit;

        to_visit.push(start_room_);
        reachable.insert(start_room_);

        // BFS to find all reachable rooms
        while (!to_visit.empty()) {
            EntityId current_id = to_visit.front();
            to_visit.pop();

            auto current_room = get_room(current_id);
            if (!current_room) continue;

            for (Direction dir : current_room->get_available_exits()) {
                auto exit = current_room->get_exit(dir);
                if (exit && exit->to_room.is_valid() && !reachable.contains(exit->to_room)) {
                    reachable.insert(exit->to_room);
                    to_visit.push(exit->to_room);
                }
            }
        }

        // Check for unreachable rooms
        for (const auto& [id, room] : rooms_) {
            if (room && !reachable.contains(id)) {
                result.unreachable_rooms++;
                result.add_warning(fmt::format("Room {} ({}) is not reachable from start room",
                                              room->name(), id));
            }
        }
    }

    return result;
}

Result<void> WorldManager::repair_world_issues() {
    auto validation = validate_world();

    if (validation.is_valid) {
        return Success();
    }

    auto logger = Log::game();
    logger->info("Attempting to repair {} world issues ({} errors, {} warnings)",
                validation.errors.size() + validation.warnings.size(),
                validation.errors.size(), validation.warnings.size());

    int repairs_made = 0;
    std::unique_lock lock(world_mutex_);

    // Repair 1: Fix orphaned rooms by adding them to a default zone or creating one
    if (validation.orphaned_rooms > 0) {
        // Find or create a "limbo" zone for orphaned rooms
        EntityId limbo_zone_id{999};  // Convention: zone 999 for limbo/orphaned content
        std::shared_ptr<Zone> limbo_zone = nullptr;

        auto it = zones_.find(limbo_zone_id);
        if (it != zones_.end()) {
            limbo_zone = it->second;
        } else {
            // Create the limbo zone with default reset settings
            // Zone::create(EntityId, name, reset_minutes, reset_mode)
            auto zone_result = Zone::create(limbo_zone_id, "Limbo", 0, ResetMode::Never);
            if (zone_result) {
                limbo_zone = std::shared_ptr<Zone>(zone_result.value().release());
                limbo_zone->set_description("A holding zone for orphaned content");
                zones_[limbo_zone_id] = limbo_zone;
                logger->info("Created limbo zone {} for orphaned rooms", limbo_zone_id);
            }
        }

        if (limbo_zone) {
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
                    limbo_zone->add_room(room->id());
                    room->set_zone_id(limbo_zone_id);
                    logger->info("Moved orphaned room {} ({}) to limbo zone", room->name(), room->id());
                    repairs_made++;
                }
            }
        }
    }

    // Repair 2: Remove or mark invalid exits
    if (validation.missing_exits > 0) {
        for (const auto& [id, room] : rooms_) {
            if (!room) continue;

            auto exits = room->get_available_exits();
            for (Direction dir : exits) {
                auto exit = room->get_exit(dir);
                if (!exit) continue;

                // Check if destination room exists
                if (!exit->to_room.is_valid() || !get_room(exit->to_room)) {
                    // Remove the invalid exit
                    room->remove_exit(dir);
                    logger->warn("Removed invalid exit {} from room {} (destination {} not found)",
                               dir, room->id(), exit->to_room);
                    repairs_made++;
                }
            }
        }
    }

    // Repair 3: Log unreachable rooms (we can't auto-fix these without knowing intent)
    if (validation.unreachable_rooms > 0) {
        logger->warn("{} rooms are unreachable from start room - manual intervention recommended",
                    validation.unreachable_rooms);
        // We don't auto-fix unreachable rooms as the fix requires understanding zone design
    }

    has_unsaved_changes_.store(true);

    logger->info("World repair completed: {} issues fixed", repairs_made);

    // Re-validate to check remaining issues
    lock.unlock();
    auto post_validation = validate_world();
    if (!post_validation.is_valid) {
        logger->warn("World still has {} remaining issues after repair",
                    post_validation.errors.size() + post_validation.warnings.size());
        for (const auto& error : post_validation.errors) {
            logger->error("Remaining issue: {}", error);
        }
    }

    return Success();
}

void WorldManager::update_statistics() {
    std::shared_lock lock(world_mutex_);

    stats_.zones_loaded = static_cast<int>(zones_.size());
    stats_.rooms_loaded = static_cast<int>(rooms_.size());

    // Count object prototypes
    stats_.objects_loaded = static_cast<int>(object_prototypes_.size());

    // Count mobile prototypes
    stats_.mobiles_loaded = static_cast<int>(mobile_prototypes_.size());

    // Additionally count spawned instances for runtime statistics
    int spawned_objects = 0;
    int spawned_mobiles = static_cast<int>(spawned_mobiles_.size());

    // Count objects in rooms
    for (const auto& [id, room] : rooms_) {
        if (room) {
            spawned_objects += static_cast<int>(room->contents().objects.size());
        }
    }

    auto logger = Log::game();
    logger->debug("Statistics updated: {} zones, {} rooms, {} object protos ({} spawned), {} mobile protos ({} spawned)",
                 stats_.zones_loaded, stats_.rooms_loaded,
                 stats_.objects_loaded, spawned_objects,
                 stats_.mobiles_loaded, spawned_mobiles);
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

        // Try to reload the file based on extension
        if (filename.ends_with(".json")) {
            // Zone files (modern format) - rooms are embedded in zone files
            auto result = load_zone_file(filename);
            if (!result) {
                logger->error("Failed to reload zone file {}: {}", filename, result.error().message);
            } else {
                logger->info("Successfully reloaded zone file: {}", filename);
            }
        } else if (filename.ends_with(".wld")) {
            // Legacy room files - attempt to parse and update rooms
            logger->info("Detected legacy room file change: {}", filename);

            std::ifstream file(filename);
            if (!file.is_open()) {
                logger->error("Failed to open room file: {}", filename);
                continue;
            }

            // Parse room JSON (legacy files may contain room array)
            try {
                nlohmann::json room_data;
                file >> room_data;

                if (room_data.is_array()) {
                    int updated_count = 0;
                    for (const auto& room_json : room_data) {
                        auto room_result = Room::from_json(room_json);
                        if (room_result) {
                            auto room = std::shared_ptr<Room>(room_result.value().release());
                            std::unique_lock lock(world_mutex_);
                            rooms_[room->id()] = room;
                            updated_count++;
                        }
                    }
                    logger->info("Updated {} rooms from {}", updated_count, filename);
                } else if (room_data.is_object()) {
                    auto room_result = Room::from_json(room_data);
                    if (room_result) {
                        auto room = std::shared_ptr<Room>(room_result.value().release());
                        std::unique_lock lock(world_mutex_);
                        rooms_[room->id()] = room;
                        logger->info("Updated room {} from {}", room->id(), filename);
                    }
                }
            } catch (const nlohmann::json::exception& e) {
                logger->error("Failed to parse room file {}: {}", filename, e.what());
            }
        }
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

Result<void> WorldManager::load_zones_from_database() {
    auto logger = Log::database();
    logger->debug("Loading zones from database");

    // Execute database query in a transaction
    auto load_result = ConnectionPool::instance().execute([&](pqxx::work& txn) -> Result<void> {
        // Load all zones
        auto zones_result = WorldQueries::load_all_zones(txn);
        if (!zones_result) {
            return std::unexpected(zones_result.error());
        }

        logger->debug("Loaded {} zones from database", zones_result->size());

        // Process each zone
        for (auto& zone_ptr : *zones_result) {
            int zone_id = zone_ptr->id().zone_id();
            logger->debug("Processing zone {}: {}", zone_id, zone_ptr->name());

            // Load rooms for this zone
            auto rooms_result = WorldQueries::load_rooms_in_zone(txn, zone_id);
            if (!rooms_result) {
                logger->error("Failed to load rooms for zone {}: {}", zone_id, rooms_result.error().message);
                return std::unexpected(rooms_result.error());
            }

            logger->debug("Loaded {} rooms for zone {}", rooms_result->size(), zone_id);

            // Register rooms in the global rooms map
            for (auto& room_ptr : *rooms_result) {
                EntityId room_id = room_ptr->id();

                // Add room to the rooms map (zone reference retrieved via zone_id as needed)
                rooms_[room_id] = std::shared_ptr<Room>(room_ptr.release());
                stats_.rooms_loaded++;
            }

            // Note: Exits are loaded in second pass after all rooms are registered
            // to properly handle cross-zone exit references

            // Load mobs for this zone
            auto mobs_result = WorldQueries::load_mobs_in_zone(txn, zone_id);
            if (!mobs_result) {
                logger->error("Failed to load mobs for zone {}: {}", zone_id, mobs_result.error().message);
                return std::unexpected(mobs_result.error());
            }

            logger->debug("Loaded {} mobs for zone {}", mobs_result->size(), zone_id);

            // Register mobile prototypes in both global vector AND lookup map
            for (auto& mob_ptr : *mobs_result) {
                EntityId mob_id = mob_ptr->id();
                // Store raw pointer in mobiles_ map for lookup during spawning
                mobiles_[mob_id] = mob_ptr.get();
                // Store ownership in mobile_prototypes_ vector
                mobile_prototypes_.push_back(std::move(mob_ptr));
                stats_.mobiles_loaded++;
            }

            // Load objects for this zone
            auto objects_result = WorldQueries::load_objects_in_zone(txn, zone_id);
            if (!objects_result) {
                logger->error("Failed to load objects for zone {}: {}", zone_id, objects_result.error().message);
                return std::unexpected(objects_result.error());
            }

            logger->debug("Loaded {} objects for zone {}", objects_result->size(), zone_id);

            // Register object prototypes in both global vector AND lookup map
            for (auto& obj_ptr : *objects_result) {
                EntityId obj_id = obj_ptr->id();
                // Store raw pointer in objects_ map for lookup during spawning
                objects_[obj_id] = obj_ptr.get();
                // Store ownership in object_prototypes_ vector
                object_prototypes_.push_back(std::move(obj_ptr));
                stats_.objects_loaded++;
            }

            // Register zone in the global zones map
            // Note: Store the ID before releasing the unique_ptr
            EntityId zone_entity_id = zone_ptr->id();
            zones_[zone_entity_id] = std::shared_ptr<Zone>(zone_ptr.release());
            stats_.zones_loaded++;
        }

        // Second pass: Load exits for all rooms now that rooms are registered
        // This allows cross-zone exit resolution
        logger->debug("Loading room exits (second pass)");
        for (const auto& [room_id, room] : rooms_) {
            int room_zone_id = room_id.zone_id();
            int room_local_id = room_id.local_id();

            auto exits_result = WorldQueries::load_room_exits(txn, room_zone_id, room_local_id);
            if (!exits_result) {
                logger->warn("Failed to load exits for room ({}, {}): {}",
                            room_zone_id, room_local_id, exits_result.error().message);
                continue;
            }

            for (const auto& exit_data : *exits_result) {
                ExitInfo exit;
                exit.to_room = exit_data.to_room;
                exit.description = exit_data.description;

                // Set first keyword as the door keyword
                if (!exit_data.keywords.empty()) {
                    exit.keyword = exit_data.keywords[0];
                }

                // Parse exit flags
                for (const auto& flag : exit_data.flags) {
                    if (flag == "IS_DOOR") {
                        exit.has_door = true;
                    } else if (flag == "CLOSED") {
                        exit.is_closed = true;
                    } else if (flag == "LOCKED") {
                        exit.is_locked = true;
                    } else if (flag == "PICKPROOF") {
                        exit.is_pickproof = true;
                    } else if (flag == "HIDDEN") {
                        exit.is_hidden = true;
                    }
                }

                // Apply exit to room
                auto set_result = room->set_exit(exit_data.direction, exit);
                if (!set_result) {
                    logger->warn("Failed to set exit {} for room ({}, {}): {}",
                                exit_data.direction, room_zone_id, room_local_id,
                                set_result.error().message);
                }
            }
        }

        // Third pass: Load mob and object resets for each zone
        logger->debug("Loading zone resets (third pass)");
        for (auto& [zone_entity_id, zone] : zones_) {
            int zone_id = zone_entity_id.zone_id();

            // Load mob resets
            auto mob_resets_result = WorldQueries::load_mob_resets_in_zone(txn, zone_id);
            if (!mob_resets_result) {
                logger->warn("Failed to load mob resets for zone {}: {}",
                            zone_id, mob_resets_result.error().message);
            } else {
                logger->debug("Loaded {} mob resets for zone {}", mob_resets_result->size(), zone_id);

                // Load all equipment for this zone at once for efficiency
                auto equipment_result = WorldQueries::load_all_mob_equipment_in_zone(txn, zone_id);
                std::unordered_map<int, std::vector<WorldQueries::MobEquipmentData>> equipment_by_reset;
                if (equipment_result) {
                    for (const auto& equip : *equipment_result) {
                        equipment_by_reset[equip.reset_id].push_back(equip);
                    }
                }

                // Convert to zone reset commands
                for (const auto& reset : *mob_resets_result) {
                    ZoneCommand cmd;
                    cmd.command_type = ZoneCommandType::Load_Mobile;
                    cmd.entity_id = reset.mob_id;
                    cmd.room_id = reset.room_id;
                    cmd.max_count = reset.max_instances;
                    cmd.if_flag = 0;  // Always execute
                    cmd.comment = reset.comment;

                    zone->add_command(cmd);

                    // Add equipment commands for this mob reset
                    auto equip_it = equipment_by_reset.find(reset.id);
                    if (equip_it != equipment_by_reset.end()) {
                        for (const auto& equip : equip_it->second) {
                            ZoneCommand equip_cmd;
                            if (equip.wear_location.empty()) {
                                equip_cmd.command_type = ZoneCommandType::Give_Object;
                            } else {
                                equip_cmd.command_type = ZoneCommandType::Equip_Object;
                                // Parse wear_location string to equipment slot number
                                // Use ObjectUtils::parse_equip_slot for case-insensitive and DB value mapping
                                auto slot = ObjectUtils::parse_equip_slot(equip.wear_location);
                                if (slot) {
                                    equip_cmd.max_count = static_cast<int>(*slot);
                                } else {
                                    logger->warn("Unknown wear_location '{}' for object {} on mob {}, defaulting to inventory",
                                                equip.wear_location, equip.object_id, reset.mob_id);
                                    // Fall back to Give_Object (inventory) if we can't parse the slot
                                    equip_cmd.command_type = ZoneCommandType::Give_Object;
                                    equip_cmd.max_count = 0;
                                }
                            }
                            equip_cmd.entity_id = equip.object_id;
                            equip_cmd.container_id = reset.mob_id;  // The mobile to equip on
                            equip_cmd.if_flag = 1;  // Only if previous command succeeded

                            zone->add_command(equip_cmd);
                        }
                    }
                }
            }

            // Load object resets
            auto obj_resets_result = WorldQueries::load_object_resets_in_zone(txn, zone_id);
            if (!obj_resets_result) {
                logger->warn("Failed to load object resets for zone {}: {}",
                            zone_id, obj_resets_result.error().message);
            } else {
                logger->debug("Loaded {} object resets for zone {}", obj_resets_result->size(), zone_id);

                // Build a map from reset_id -> object_id for container lookups
                std::unordered_map<int, EntityId> reset_to_object;
                for (const auto& reset : *obj_resets_result) {
                    reset_to_object[reset.id] = reset.object_id;
                }

                for (const auto& reset : *obj_resets_result) {
                    ZoneCommand cmd;
                    if (reset.container_reset_id > 0) {
                        cmd.command_type = ZoneCommandType::Put_Object;
                        // Look up the container's object_id from the referenced reset
                        auto container_it = reset_to_object.find(reset.container_reset_id);
                        if (container_it != reset_to_object.end()) {
                            cmd.container_id = container_it->second;
                        } else {
                            logger->warn("Object reset {} references non-existent container reset {}",
                                        reset.id, reset.container_reset_id);
                            cmd.container_id = INVALID_ENTITY_ID;
                        }
                    } else {
                        cmd.command_type = ZoneCommandType::Load_Object;
                    }
                    cmd.entity_id = reset.object_id;
                    cmd.room_id = reset.room_id;
                    cmd.max_count = reset.max_instances;
                    cmd.if_flag = 0;  // Always execute
                    cmd.comment = reset.comment;

                    zone->add_command(cmd);
                }
            }
        }

        // Fourth pass: Set up zone callbacks for mob/object spawning
        logger->debug("Setting up zone callbacks (fourth pass)");
        for (auto& [zone_entity_id, zone] : zones_) {
            setup_zone_callbacks(zone);
        }

        // Fifth pass: Load shops and register shopkeepers
        logger->debug("Loading shops (fifth pass)");
        auto shops_result = WorldQueries::load_all_shops(txn);
        if (!shops_result) {
            logger->warn("Failed to load shops: {}", shops_result.error().message);
        } else {
            auto& shop_manager = ShopManager::instance();
            int shops_loaded = 0;

            for (const auto& shop_data : *shops_result) {
                // Find the mob prototype for this shopkeeper
                auto mob_it = mobiles_.find(shop_data.keeper_id);
                if (mob_it == mobiles_.end()) {
                    logger->warn("Shop {} references non-existent mob {}",
                                shop_data.id, shop_data.keeper_id);
                    continue;
                }

                // Mark the mob prototype as a shopkeeper
                mob_it->second->set_shopkeeper(true);

                // Create the shop with the keeper's mob ID
                auto shop = std::make_unique<Shopkeeper>(shop_data.keeper_id);

                // Use the mob's name for the shop name
                shop->set_name(fmt::format("{}'s Shop", mob_it->second->name()));

                // Set shop rates (buy_profit is what shopkeeper charges, sell_profit is what they pay)
                shop->set_buy_rate(shop_data.buy_profit);
                shop->set_sell_rate(shop_data.sell_profit);

                // Load items for this shop
                auto items_result = WorldQueries::load_shop_items(txn,
                    shop_data.keeper_id.zone_id(), shop_data.id);
                if (items_result) {
                    for (const auto& item_data : *items_result) {
                        // Find the object prototype
                        auto obj_it = objects_.find(item_data.object_id);
                        if (obj_it == objects_.end()) {
                            logger->warn("Shop {} item references non-existent object {}",
                                        shop_data.id, item_data.object_id);
                            continue;
                        }

                        // Create shop item from prototype
                        const auto* obj_proto = obj_it->second;
                        ShopItem shop_item;
                        shop_item.prototype_id = item_data.object_id;
                        shop_item.name = obj_proto->name();
                        shop_item.description = obj_proto->short_description();
                        shop_item.cost = obj_proto->value();
                        shop_item.stock = item_data.amount;
                        shop_item.max_stock = item_data.amount;

                        shop->add_item(shop_item);
                    }
                }

                // Register the shop with ShopManager
                shop_manager.register_shopkeeper(shop_data.keeper_id, std::move(shop));
                shops_loaded++;
            }

            logger->info("Loaded {} shops from database", shops_loaded);
        }

        logger->info("Successfully loaded {} zones, {} rooms, {} mobs, {} objects from database",
                    stats_.zones_loaded, stats_.rooms_loaded, stats_.mobiles_loaded, stats_.objects_loaded);
        return Success();
    });

    return load_result;
}

Result<void> WorldManager::load_rooms_from_directory(const std::string& /* room_dir */) {
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
                                                        std::shared_ptr<Room> /* from_room */,
                                                        std::shared_ptr<Room> to_room,
                                                        Direction /* direction */) const {
    if (!to_room->can_accommodate(actor.get())) {
        return MovementResult("Destination room is full");
    }

    // Check if actor is a Player to access god_level
    auto* player = dynamic_cast<const Player*>(actor.get());
    bool is_god = player && player->is_god();
    bool is_mobile = (dynamic_cast<const Mobile*>(actor.get()) != nullptr) && !player;

    // Check Godroom - only immortals can enter
    if (to_room->has_flag(RoomFlag::Godroom) && !is_god) {
        return MovementResult("Only immortals may enter this divine sanctum");
    }

    // Check NoMob - mobiles (NPCs) cannot enter
    if (to_room->has_flag(RoomFlag::NoMob) && is_mobile) {
        return MovementResult("Mobiles cannot enter this area");
    }

    // Check Tunnel - only one person at a time
    if (to_room->has_flag(RoomFlag::Tunnel) && !to_room->contents().actors.empty()) {
        // Allow gods to bypass tunnel restriction
        if (!is_god) {
            return MovementResult("The passage is too narrow for more than one person");
        }
    }

    // Check Private room - maximum 2 occupants
    if (to_room->has_flag(RoomFlag::Private)) {
        if (to_room->contents().actors.size() >= 2 && !is_god) {
            return MovementResult("This is a private room");
        }
    }

    // Check zone level restrictions
    auto zone = get_zone(to_room->zone_id());
    if (zone && !is_god) {
        int actor_level = actor->stats().level;

        if (!zone->allows_level(actor_level)) {
            if (actor_level < zone->min_level()) {
                return MovementResult(fmt::format(
                    "You must be at least level {} to enter this area", zone->min_level()));
            } else {
                return MovementResult(fmt::format(
                    "This area is for characters level {} and below", zone->max_level()));
            }
        }
    }

    // Check death trap (warn but don't block - player's choice)
    // Death trap logic is handled elsewhere after movement completes

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

    // Check actor-specific restrictions
    auto* player = dynamic_cast<const Player*>(actor.get());
    bool is_god = player && player->is_god();
    bool is_mobile = (dynamic_cast<const Mobile*>(actor.get()) != nullptr) && !player;

    // Gods can go anywhere
    if (is_god) {
        return true;
    }

    // Godroom restriction
    if (room->has_flag(RoomFlag::Godroom)) {
        return false;
    }

    // NoMob restriction for NPCs
    if (room->has_flag(RoomFlag::NoMob) && is_mobile) {
        return false;
    }

    // Check zone level restrictions
    auto zone = get_zone(room->zone_id());
    if (zone) {
        int actor_level = actor->stats().level;
        if (!zone->allows_level(actor_level)) {
            return false;
        }
    }

    // Check sector-based restrictions
    SectorType sector = room->sector_type();

    // Water sectors require swimming ability (unless flying or walking on water)
    if (RoomUtils::requires_swimming(sector)) {
        // Check if actor can swim or has water walking ability
        if (!actor->has_flag(ActorFlag::Waterwalk) &&
            !actor->has_flag(ActorFlag::Flying)) {
            // Check if actor can swim (basic ability for now)
            // In a full implementation, we'd check SKILL_SWIM
            return false;
        }
    }

    // Flying sector requires flying ability
    if (sector == SectorType::Flying) {
        if (!actor->has_flag(ActorFlag::Flying)) {
            return false;
        }
    }

    // Underwater requires water breathing
    if (sector == SectorType::Underwater) {
        if (!actor->has_flag(ActorFlag::Underwater_Breathing)) {
            return false;
        }
    }

    return true;
}

int WorldManager::get_movement_cost(std::shared_ptr<Room> /* from_room */, std::shared_ptr<Room> to_room,
                                   std::shared_ptr<Actor> actor) const {
    if (!to_room) {
        return INVALID_ROOM_MOVEMENT_COST;
    }

    int base_cost = RoomUtils::get_movement_cost(to_room->sector_type());

    // Apply actor-specific modifiers
    if (actor) {
        SectorType sector = to_room->sector_type();

        // Flying reduces movement cost in most sectors
        if (actor->has_flag(ActorFlag::Flying)) {
            // Flying is efficient in air and reduces ground movement costs
            if (sector == SectorType::Flying) {
                base_cost = FLYING_OPTIMAL_COST;
            } else if (!RoomUtils::is_water_sector(sector)) {
                base_cost = std::max(1, base_cost / 2);  // Half cost on ground while flying
            }
        }

        // Water walking reduces water sector costs
        if (actor->has_flag(ActorFlag::Waterwalk)) {
            if (RoomUtils::is_water_sector(sector)) {
                base_cost = std::max(1, base_cost - 2);  // Reduced water movement cost
            }
        }

        // TODO: Add Haste flag support when spell system is integrated
        // Haste would reduce all movement costs by 1

        // Encumbrance increases movement cost
        // Higher weight carried = higher cost modifier
        float encumbrance_ratio = 0.0f;
        if (actor->max_carry_weight() > 0) {
            encumbrance_ratio = static_cast<float>(actor->current_carry_weight()) /
                               static_cast<float>(actor->max_carry_weight());
        }
        if (encumbrance_ratio > 0.75f) {
            base_cost += 2;  // Heavy load
        } else if (encumbrance_ratio > 0.5f) {
            base_cost += 1;  // Medium load
        }

        // Low constitution increases cost slightly
        if (actor->stats().constitution < 10) {
            base_cost += 1;
        }

        // Sneaking increases movement cost
        if (actor->has_flag(ActorFlag::Sneak)) {
            base_cost += 1;  // Moving carefully takes more effort
        }
    }

    return std::max(1, base_cost);  // Minimum cost of 1
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
    new_mobile->set_keywords(prototype->keywords());
    new_mobile->set_short_description(prototype->short_description());
    new_mobile->set_description(prototype->description());
    new_mobile->set_ground(prototype->ground());  // Room description (shown when mob is in room)
    new_mobile->stats() = prototype->stats();
    new_mobile->set_aggressive(prototype->is_aggressive());
    new_mobile->set_aggression_level(prototype->aggression_level());

    // Copy mobile-specific properties
    new_mobile->set_race(prototype->race());
    new_mobile->set_gender(prototype->gender());
    new_mobile->set_size(prototype->size());
    new_mobile->set_life_force(prototype->life_force());
    new_mobile->set_composition(prototype->composition());
    new_mobile->set_damage_type(prototype->damage_type());

    // Copy HP dice and calculate HP for this instance
    new_mobile->set_hp_dice(prototype->hp_dice_num(), prototype->hp_dice_size(), prototype->hp_dice_bonus());
    new_mobile->calculate_hp_from_dice();

    // Copy bare hand damage dice
    new_mobile->set_bare_hand_damage(prototype->bare_hand_damage_dice_num(),
                                      prototype->bare_hand_damage_dice_size(),
                                      prototype->bare_hand_damage_dice_bonus());

    // Copy special role flags
    new_mobile->set_teacher(prototype->is_teacher());
    new_mobile->set_shopkeeper(prototype->is_shopkeeper());
    new_mobile->set_class_id(prototype->class_id());
    new_mobile->set_prototype_id(prototype->id());  // Track prototype for shop lookups

    // Find appropriate room for spawning - for now, spawn in the first room of the zone
    auto zone_rooms = get_rooms_in_zone(zone_id);
    if (zone_rooms.empty()) {
        logger->warn("No rooms available in zone {} for spawning mobile {}", zone_id, prototype->name());
        return std::unexpected(Errors::InvalidState("No rooms in zone"));
    }
    
    // For the test zone, spawn specific mobs in specific rooms
    std::shared_ptr<Room> spawn_room = nullptr;
    
    if (prototype->name() == "a practice dummy") {
        // Spawn practice dummy in training room
        spawn_room = get_room(EntityId{TEST_TRAINING_ROOM_ID});
    } else if (prototype->name() == "a training guard") {
        // Spawn training guard in starting room
        spawn_room = get_room(EntityId{TEST_STARTING_ROOM_ID});
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
    
    logger->debug("Spawned mobile '{}' in room '{}' ({})",
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
    
    // Set up object removal callback
    zone->set_remove_object_callback([this](EntityId object_id, EntityId room_id) -> bool {
        auto room = get_room(room_id);
        if (!room) {
            return false;
        }
        return room->remove_object(object_id);
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
    new_mobile->set_ground(prototype->ground());  // Room description (shown when mob is in room)
    new_mobile->stats() = prototype->stats();
    new_mobile->set_aggressive(prototype->is_aggressive());
    new_mobile->set_aggression_level(prototype->aggression_level());

    // Copy mobile-specific properties
    new_mobile->set_race(prototype->race());
    new_mobile->set_gender(prototype->gender());
    new_mobile->set_size(prototype->size());
    new_mobile->set_life_force(prototype->life_force());
    new_mobile->set_composition(prototype->composition());
    new_mobile->set_damage_type(prototype->damage_type());

    // Copy HP dice and calculate HP for this instance
    new_mobile->set_hp_dice(prototype->hp_dice_num(), prototype->hp_dice_size(), prototype->hp_dice_bonus());
    new_mobile->calculate_hp_from_dice();

    // Copy bare hand damage dice
    new_mobile->set_bare_hand_damage(prototype->bare_hand_damage_dice_num(),
                                      prototype->bare_hand_damage_dice_size(),
                                      prototype->bare_hand_damage_dice_bonus());

    // Copy special role flags
    new_mobile->set_teacher(prototype->is_teacher());
    new_mobile->set_shopkeeper(prototype->is_shopkeeper());
    new_mobile->set_class_id(prototype->class_id());
    new_mobile->set_prototype_id(prototype->id());  // Track prototype for shop lookups

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
    
    // Check if this is an encoded equipment request
    // Equipment encoding: zone_id = slot (0-31), local_id = (mobile_zone << 16) | mobile_local_id
    // Detection: slot must be valid (0-31) AND packed_mobile looks like a packed value (>= 65536)
    // since any mobile in zone >= 1 will have packed_mobile >= 65536 (1 << 16 = 65536)
    uint32_t potential_slot = room_id.zone_id();
    uint32_t packed_mobile = room_id.local_id();
    bool is_equipment_request = (potential_slot < 32 && packed_mobile >= 65536);

    // First check if this is a simple inventory request (room_id is a mobile's EntityId)
    if (!is_equipment_request) {
        // Check if object is suitable for mobile inventory (not too heavy, appropriate type)
        // Allow OTHER type objects since they include toys, dolls, and other items mobiles can carry
        bool suitable_for_mobile = (new_object->weight() <= MOBILE_INVENTORY_MAX_WEIGHT &&
                                   new_object->type() != ObjectType::Fountain);

        if (suitable_for_mobile) {
            // This might be an inventory request - try to find a mobile with this EntityId
            // room_id is passed directly from zone.cpp (the mobile's actual EntityId)
            std::shared_ptr<Mobile> target_mobile = find_spawned_mobile(room_id);

            if (target_mobile) {
                // This is an inventory request - give the item to the mobile
                auto shared_object = std::shared_ptr<Object>(new_object.release());

                auto give_result = target_mobile->give_item(shared_object);
                if (!give_result) {
                    logger->error("Failed to give object {} to mobile {}: {}",
                                object_id, room_id, give_result.error().message);
                    return nullptr;
                }

                logger->debug("Successfully gave object {} to mobile {} inventory",
                             object_id, room_id);
                // Register for container lookups (e.g., bags given to mobiles)
                register_spawned_object(shared_object, object_id);
                return shared_object;
            }
        }
        // If unsuitable for mobile or no mobile found, continue to standard room placement logic
    }

    // If this is an equipment request (zone_id 0-31 = slot, local_id = packed mobile ID)
    if (is_equipment_request) {
        // Decode the mobile ID from the packed local_id
        // local_id = (mobile_zone_id << 16) | mobile_local_id
        uint32_t mobile_zone = packed_mobile >> 16;
        uint32_t mobile_local_id = packed_mobile & 0xFFFF;
        EntityId mobile_id(mobile_zone, mobile_local_id);
        
        // Use efficient O(1) lookup for the mobile
        std::shared_ptr<Mobile> target_mobile = find_spawned_mobile(mobile_id);
        
        if (!target_mobile) {
            logger->error("Cannot equip object {} - mobile {} not found", object_id, mobile_id);
            return nullptr;
        }
        
        // Convert equipment slot number to EquipSlot enum
        EquipSlot equip_slot = static_cast<EquipSlot>(potential_slot);
        
        auto shared_object = std::shared_ptr<Object>(new_object.release());
        
        // Set the correct equip slot on the object before equipping
        shared_object->set_equip_slot(equip_slot);
        
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
                     object_id, mobile_id, potential_slot);
        // Register for container lookups (e.g., equipped containers)
        register_spawned_object(shared_object, object_id);
        return shared_object;
    }
    
    // Check if this is a container placement (room_id is a container's prototype ID)
    // This happens for Put_Object commands where room_id is actually the container's EntityId
    auto container = find_last_spawned_object(room_id);
    if (container && container->is_container()) {
        // This is a container placement - cast to Container to access add_item
        auto* container_ptr = dynamic_cast<Container*>(container.get());
        if (container_ptr) {
            auto shared_object = std::shared_ptr<Object>(new_object.release());

            // Add to container's contents using the proper add_item method
            auto add_result = container_ptr->add_item(shared_object);
            if (!add_result) {
                logger->warn("Failed to place object {} in container {}: {}",
                            object_id, room_id, add_result.error().message);
                return nullptr;
            }

            logger->debug("Placed object {} in container {}", object_id, room_id);

            // Register this object as well for potential nested containers
            register_spawned_object(shared_object, object_id);
            return shared_object;
        }
    }

    // Standard room placement
    auto room = get_room(room_id);
    if (!room) {
        logger->error("Cannot place object {} - room {} not found", object_id, room_id);
        return nullptr;
    }

    auto shared_object = std::shared_ptr<Object>(new_object.release());
    room->add_object(shared_object);

    // Register this spawned object for future container lookups
    // This allows Put_Object commands to find this container
    register_spawned_object(shared_object, object_id);

    return shared_object;
}

std::shared_ptr<Object> WorldManager::create_object_instance(EntityId prototype_id) {
    auto logger = Log::game();

    // Find object prototype
    auto object_it = objects_.find(prototype_id);
    if (object_it == objects_.end()) {
        logger->error("Cannot create object instance {} - prototype not found", prototype_id);
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
    new_object->set_liquid_info(prototype->liquid_info());

    return std::shared_ptr<Object>(new_object.release());
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
    new_mobile->set_keywords(prototype->keywords());
    new_mobile->set_short_description(prototype->short_description());
    new_mobile->set_description(prototype->description());
    new_mobile->set_ground(prototype->ground());  // Room description (shown when mob is in room)
    new_mobile->stats() = prototype->stats();
    new_mobile->set_aggressive(prototype->is_aggressive());
    new_mobile->set_aggression_level(prototype->aggression_level());

    // Copy mobile-specific properties
    new_mobile->set_race(prototype->race());
    new_mobile->set_gender(prototype->gender());
    new_mobile->set_size(prototype->size());
    new_mobile->set_life_force(prototype->life_force());
    new_mobile->set_composition(prototype->composition());
    new_mobile->set_damage_type(prototype->damage_type());

    // Copy HP dice and calculate HP for this instance
    new_mobile->set_hp_dice(prototype->hp_dice_num(), prototype->hp_dice_size(), prototype->hp_dice_bonus());
    new_mobile->calculate_hp_from_dice();

    // Copy bare hand damage dice
    new_mobile->set_bare_hand_damage(prototype->bare_hand_damage_dice_num(),
                                      prototype->bare_hand_damage_dice_size(),
                                      prototype->bare_hand_damage_dice_bonus());

    // Copy special role flags
    new_mobile->set_teacher(prototype->is_teacher());
    new_mobile->set_shopkeeper(prototype->is_shopkeeper());
    new_mobile->set_class_id(prototype->class_id());
    new_mobile->set_prototype_id(prototype->id());  // Track prototype for shop lookups

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
    
    logger->debug("Spawned mobile '{}' in room '{}' ({})",
                prototype->name(), spawn_room->name(), spawn_room->id());

    return Success();
}

// Mobile instance tracking for efficient lookups
void WorldManager::register_spawned_mobile(std::shared_ptr<Mobile> mobile) {
    if (!mobile) return;
    
    std::lock_guard<std::shared_mutex> lock(world_mutex_);
    spawned_mobiles_[mobile->id()] = mobile;
    
    auto logger = Log::game();
    logger->debug("Registered spawned mobile {} ({})", mobile->display_name(), mobile->id());
}

void WorldManager::unregister_spawned_mobile(EntityId mobile_id) {
    std::lock_guard<std::shared_mutex> lock(world_mutex_);
    auto it = spawned_mobiles_.find(mobile_id);
    if (it != spawned_mobiles_.end()) {
        auto logger = Log::game();
        logger->debug("Unregistered spawned mobile {} ({})", it->second->display_name(), mobile_id);
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

// Object instance tracking for container lookups
void WorldManager::register_spawned_object(std::shared_ptr<Object> object, EntityId prototype_id) {
    if (!object) return;

    std::lock_guard<std::shared_mutex> lock(world_mutex_);
    last_spawned_objects_[prototype_id] = object;

    auto logger = Log::game();
    logger->debug("Registered spawned object {} (prototype {})", object->display_name(), prototype_id);
}

std::shared_ptr<Object> WorldManager::find_last_spawned_object(EntityId prototype_id) const {
    std::shared_lock<std::shared_mutex> lock(world_mutex_);
    auto it = last_spawned_objects_.find(prototype_id);
    return (it != last_spawned_objects_.end()) ? it->second : nullptr;
}

void WorldManager::clear_spawned_objects() {
    std::lock_guard<std::shared_mutex> lock(world_mutex_);
    last_spawned_objects_.clear();

    auto logger = Log::game();
    logger->debug("Cleared spawned object tracking");
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
        
        [[maybe_unused]] auto weather_effects = Weather().get_weather_effects(zone_id);
        
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
        for (size_t i = 0; i < magic_enum::enum_count<SectorType>(); ++i) {
            auto sector = static_cast<SectorType>(i);
            auto rooms = world.find_rooms_by_sector(sector);
            std::string key = fmt::format("rooms_{}", magic_enum::enum_name(sector));
            metrics[key] = static_cast<int>(rooms.size());
        }
        
        // Count zones by flag
        for (size_t i = 0; i < magic_enum::enum_count<ZoneFlag>(); ++i) {
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