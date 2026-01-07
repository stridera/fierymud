#pragma once

#include "../core/ids.hpp"
#include "../core/result.hpp"
#include "room.hpp"
#include "zone.hpp"

#include <atomic>
#include <chrono>
#include <filesystem>
#include <functional>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <string_view>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

// Forward declarations
class Actor;
class Object;
class Mobile;

/**
 * World management system for FieryMUD.
 *
 * Coordinates all world-related operations:
 * - Room and zone loading/management
 * - World state persistence
 * - Zone reset scheduling
 * - World integrity validation
 * - Performance monitoring
 * - Hot-reloading of world data
 */

/** World loading statistics */
struct WorldStats {
    std::chrono::steady_clock::time_point startup_time;
    std::chrono::steady_clock::time_point last_save;

    int zones_loaded = 0;
    int rooms_loaded = 0;
    int objects_loaded = 0;
    int mobiles_loaded = 0;

    int failed_zone_loads = 0;
    int failed_room_loads = 0;

    std::chrono::milliseconds load_time{0};
    std::chrono::milliseconds save_time{0};

    /** Get uptime in seconds */
    std::chrono::seconds uptime() const {
        return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - startup_time);
    }

    /** Get time since last save */
    std::chrono::seconds time_since_save() const {
        return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - last_save);
    }
};

/** World validation results */
struct ValidationResult {
    bool is_valid = true;
    std::vector<std::string> errors;
    std::vector<std::string> warnings;

    int orphaned_rooms = 0;    // Rooms not in any zone
    int missing_exits = 0;     // Exits pointing to non-existent rooms
    int unreachable_rooms = 0; // Rooms not reachable from start room

    void add_error(std::string_view message) {
        errors.emplace_back(message);
        is_valid = false;
    }

    void add_warning(std::string_view message) { warnings.emplace_back(message); }

    std::string summary() const {
        return fmt::format("Validation: {} errors, {} warnings", errors.size(), warnings.size());
    }
};

/** Movement result information */
struct MovementResult {
    bool success = false;
    std::string failure_reason;
    EntityId from_room = INVALID_ENTITY_ID;
    EntityId to_room = INVALID_ENTITY_ID;
    Direction direction = Direction::None;

    MovementResult() = default;
    MovementResult(bool success) : success(success) {}
    MovementResult(std::string_view reason) : success(false), failure_reason(reason) {}
};

/** World event callback types */
using RoomEnterCallback = std::function<void(std::shared_ptr<Actor>, std::shared_ptr<Room>)>;
using RoomExitCallback = std::function<void(std::shared_ptr<Actor>, std::shared_ptr<Room>)>;
using ZoneResetCallback = std::function<void(std::shared_ptr<Zone>)>;
using ObjectCreateCallback = std::function<void(std::shared_ptr<Object>)>;

/** World Manager singleton class */
class WorldManager {
  public:
    /** Get singleton instance */
    static WorldManager &instance();

    /** Initialize world manager */
    Result<void> initialize(const std::string &world_path = "lib/world", bool load_from_files = true);

    /** Shutdown world manager */
    void shutdown();
    void clear_state();

    // Shutdown Request System
    /** Request a graceful shutdown after specified seconds (0 = immediate) */
    void request_shutdown(int seconds_from_now = 0, std::string_view reason = "");

    /** Cancel a pending shutdown */
    void cancel_shutdown();

    /** Check if shutdown has been requested */
    bool is_shutdown_requested() const { return shutdown_requested_.load(); }

    /** Get the time at which shutdown was requested to occur */
    std::chrono::steady_clock::time_point get_shutdown_time() const { return shutdown_time_; }

    /** Get the reason for shutdown */
    std::string get_shutdown_reason() const { return shutdown_reason_; }

    /** Check if shutdown time has passed (ready to shutdown now) */
    bool should_shutdown_now() const;

    // World Loading
    Result<void> load_world();
    Result<void> load_zone_file(const std::string &filename);
    Result<void> reload_zone(EntityId zone_id);
    Result<void> reload_all_zones();

    // Room Management
    std::shared_ptr<Room> get_room(EntityId room_id) const;
    std::shared_ptr<Room> get_first_available_room() const;
    Result<void> add_room(std::shared_ptr<Room> room);
    void remove_room(EntityId room_id);

    std::vector<std::shared_ptr<Room>> find_rooms_by_keyword(std::string_view keyword) const;
    std::vector<std::shared_ptr<Room>> get_rooms_in_zone(EntityId zone_id) const;

    EntityId get_start_room() const { return start_room_; }
    void set_start_room(EntityId room_id) { start_room_ = room_id; }

    // Zone Management
    std::shared_ptr<Zone> get_zone(EntityId zone_id) const;
    Result<void> add_zone(std::shared_ptr<Zone> zone);
    void remove_zone(EntityId zone_id);

    std::vector<std::shared_ptr<Zone>> get_all_zones() const;
    std::shared_ptr<Zone> find_zone_by_name(std::string_view name) const;

    // Movement System
    MovementResult move_actor(std::shared_ptr<Actor> actor, Direction direction);
    MovementResult move_actor_to_room(std::shared_ptr<Actor> actor, EntityId room_id);

    bool can_move_direction(std::shared_ptr<Actor> actor, Direction direction) const;
    std::shared_ptr<Room> get_room_in_direction(EntityId from_room, Direction direction) const;

    /**
     * Check if actor should fall in current room and handle falling.
     * If actor is in a Flying sector room without flying ability, they will fall
     * down through any down exits until reaching solid ground.
     * @param actor The actor to check
     * @return True if actor fell, false if no falling occurred
     */
    bool check_and_handle_falling(std::shared_ptr<Actor> actor);

    /**
     * Check if object should fall in current room and handle falling.
     * If object is in a Flying sector room, it will fall down through
     * any down exits until reaching solid ground.
     * @param object The object to check
     * @param room The room the object is in
     * @return True if object fell, false if no falling occurred
     */
    bool check_and_handle_object_falling(std::shared_ptr<Object> object, std::shared_ptr<Room> room);

    // World State Management
    Result<void> load_world_state();
    bool has_unsaved_changes() const { return has_unsaved_changes_; }

    // Zone Reset System
    void process_zone_resets();
    void force_zone_reset(EntityId zone_id);
    void schedule_zone_reset(EntityId zone_id, std::chrono::seconds delay);

    // World Validation
    ValidationResult validate_world() const;
    Result<void> repair_world_issues();

    // Statistics and Monitoring
    const WorldStats &stats() const { return stats_; }
    void update_statistics();

    size_t room_count() const { return rooms_.size(); }
    size_t zone_count() const { return zones_.size(); }

    // Event System
    void set_room_enter_callback(RoomEnterCallback callback) { room_enter_callback_ = std::move(callback); }
    void set_room_exit_callback(RoomExitCallback callback) { room_exit_callback_ = std::move(callback); }
    void set_zone_reset_callback(ZoneResetCallback callback) { zone_reset_callback_ = std::move(callback); }
    void set_object_create_callback(ObjectCreateCallback callback) { object_create_callback_ = std::move(callback); }

    // World Search and Queries
    std::vector<std::shared_ptr<Room>> find_rooms_with_flag(RoomFlag flag) const;
    std::vector<std::shared_ptr<Zone>> find_zones_with_flag(ZoneFlag flag) const;

    std::vector<std::shared_ptr<Room>> find_rooms_by_sector(SectorType sector) const;
    std::vector<std::shared_ptr<Room>> find_rooms_in_level_range(int min_level, int max_level) const;

    // Path Finding
    std::vector<Direction> find_path(EntityId from_room, EntityId to_room,
                                     std::shared_ptr<Actor> actor = nullptr) const;
    int calculate_distance(EntityId from_room, EntityId to_room) const;

    // Hot Reloading
    void enable_file_watching(bool enabled = true);
    void check_for_file_changes();

    // Debugging and Administration
    std::string get_world_summary() const;
    std::vector<std::string> get_zone_list() const;
    std::vector<std::string> get_orphaned_rooms() const;

    void dump_world_state(const std::string &filename) const;
    Result<void> import_world_state(const std::string &filename);

    // Object Instance Creation (for player inventory loading)
    std::shared_ptr<Object> create_object_instance(EntityId prototype_id);

    // Mobile Instance Creation (for load command and zone resets)
    std::shared_ptr<Mobile> spawn_mobile_to_room(EntityId prototype_id, EntityId room_id);

    // Mobile Despawning (for mob death)
    void despawn_mobile(EntityId mobile_id);

    // Mobile Iteration (for scripting triggers, effects, etc.)
    template<typename Func>
    void for_each_mobile(Func&& func) const {
        for (const auto& [id, mobile] : spawned_mobiles_) {
            if (mobile) {
                func(mobile);
            }
        }
    }

    // Get all spawned mobiles (for mob activity/AI processing)
    const std::unordered_map<EntityId, std::shared_ptr<Mobile>>& spawned_mobiles() const {
        return spawned_mobiles_;
    }

    // Prototype Access (for load command and similar admin features)
    Mobile* get_mobile_prototype(EntityId prototype_id) const;
    Object* get_object_prototype(EntityId prototype_id) const;

    // Weather Integration
    void update_weather_system(std::chrono::minutes elapsed);
    void initialize_weather_callbacks();

    // Tick Systems
    void tick_regen_all();      // Fast tick (every 4s) - HP/move regen, DoT/HoT
    void tick_hour_all();       // Hour tick (every 75s) - effect durations, conditions
    void tick_all_effects();    // Legacy compatibility - calls tick_regen_all()

  private:
    WorldManager() = default;
    ~WorldManager() = default;
    WorldManager(const WorldManager &) = delete;
    WorldManager &operator=(const WorldManager &) = delete;

    // Core data structures
    std::unordered_map<EntityId, std::shared_ptr<Room>> rooms_;
    std::unordered_map<EntityId, std::shared_ptr<Zone>> zones_;

    // Object and mobile prototypes (for spawning instances)
    // Using shared_ptr for both storage and lookup to prevent dangling pointer issues
    std::unordered_map<EntityId, std::shared_ptr<Object>> objects_;  // Quick lookup by ID
    std::unordered_map<EntityId, std::shared_ptr<Mobile>> mobiles_;  // Quick lookup by ID
    std::vector<std::shared_ptr<Object>> object_prototypes_;         // Owned prototypes
    std::vector<std::shared_ptr<Mobile>> mobile_prototypes_;         // Owned prototypes

    // Spawned instance tracking (for efficient equipment lookups)
    std::unordered_map<EntityId, std::shared_ptr<Mobile>> spawned_mobiles_; // Live mobile instances by ID

    EntityId start_room_ = INVALID_ENTITY_ID;
    std::string world_path_;

    // State tracking
    std::atomic<bool> initialized_{false};
    std::atomic<bool> has_unsaved_changes_{false};
    mutable std::shared_mutex world_mutex_;

    // Shutdown request tracking
    std::atomic<bool> shutdown_requested_{false};
    std::chrono::steady_clock::time_point shutdown_time_{};
    std::string shutdown_reason_;

    // Statistics
    WorldStats stats_;

    // Event callbacks
    RoomEnterCallback room_enter_callback_;
    RoomExitCallback room_exit_callback_;
    ZoneResetCallback zone_reset_callback_;
    ObjectCreateCallback object_create_callback_;

    // File watching
    std::atomic<bool> file_watching_enabled_{false};
    std::unordered_map<std::string, std::filesystem::file_time_type> file_timestamps_;

    // Zone reset scheduling
    std::unordered_map<EntityId, std::chrono::steady_clock::time_point> scheduled_resets_;

    // Helper methods
    Result<void> load_zones_from_directory(const std::string &zone_dir);
    Result<void> load_zones_from_database();  // Load zones from PostgreSQL database
    Result<void> load_rooms_from_directory(const std::string &room_dir);

    // Mobile spawning
    Result<void> spawn_mobile_in_zone(Mobile *prototype, EntityId zone_id);

    // Zone callback setup
    void setup_zone_callbacks(std::shared_ptr<Zone> zone);
    std::shared_ptr<Mobile> spawn_mobile_for_zone(EntityId mobile_id, EntityId room_id);
    std::shared_ptr<Object> spawn_object_for_zone(EntityId object_id, EntityId room_id);
    Result<void> spawn_mobile_in_specific_room(Mobile *prototype, EntityId room_id);

    // Mobile instance tracking for efficient lookups
    void register_spawned_mobile(std::shared_ptr<Mobile> mobile);
    void unregister_spawned_mobile(EntityId mobile_id);
    std::shared_ptr<Mobile> find_spawned_mobile(EntityId mobile_id) const;
    void cleanup_zone_mobiles(EntityId zone_id);

    void validate_room_exits(std::shared_ptr<Room> room, ValidationResult &result) const;
    void validate_zone_integrity(std::shared_ptr<Zone> zone, ValidationResult &result) const;

    MovementResult check_movement_restrictions(std::shared_ptr<Actor> actor, std::shared_ptr<Room> from_room,
                                               std::shared_ptr<Room> to_room, Direction direction) const;

    void notify_room_enter(std::shared_ptr<Actor> actor, std::shared_ptr<Room> room);
    void notify_room_exit(std::shared_ptr<Actor> actor, std::shared_ptr<Room> room);
    void notify_zone_reset(std::shared_ptr<Zone> zone);
    void notify_object_create(std::shared_ptr<Object> object);

    void update_file_timestamps();
    std::vector<std::string> get_changed_files();

    // Pathfinding helpers
    std::vector<Direction> reconstruct_path(const std::unordered_map<EntityId, EntityId> &came_from,
                                            const std::unordered_map<EntityId, Direction> &directions, EntityId start,
                                            EntityId goal) const;

    bool is_passable_for_actor(std::shared_ptr<Room> room, std::shared_ptr<Actor> actor) const;
    int get_movement_cost(std::shared_ptr<Room> from_room, std::shared_ptr<Room> to_room,
                          std::shared_ptr<Actor> actor) const;
};

/** World management utility functions */
namespace WorldUtils {
/** Check if world directory structure exists */
bool validate_world_directory(const std::string &world_path);

/** Create default world directory structure */
Result<void> create_world_directory(const std::string &world_path);

/** Get all zone files in directory */
std::vector<std::string> get_zone_files(const std::string &zone_dir);

/** Get all room files in directory */
std::vector<std::string> get_room_files(const std::string &room_dir);

/** Backup world data */
Result<void> backup_world_data(const std::string &world_path, const std::string &backup_path);

/** Restore world data from backup */
Result<void> restore_world_data(const std::string &backup_path, const std::string &world_path);

/** Calculate world statistics */
std::unordered_map<std::string, int> calculate_world_metrics(const WorldManager &world);

/** Export world to different format */
Result<void> export_world(const WorldManager &world, const std::string &filename, std::string_view format = "json");

/** Import world from different format */
Result<void> import_world(WorldManager &world, const std::string &filename, std::string_view format = "json");
} // namespace WorldUtils

/** Global world manager access */
inline WorldManager &World() { return WorldManager::instance(); }