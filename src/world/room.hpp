#pragma once

#include "core/entity.hpp"
#include "core/result.hpp"
#include "core/ids.hpp"
#include "database/generated/db_room.hpp"

#include <memory>
#include <vector>
#include <unordered_map>
#include <string>
#include <string_view>
#include <optional>

// Forward declarations
class Actor;
class Object;
class Zone;

/**
 * Modern room system for FieryMUD.
 *
 * Replaces legacy room structures with:
 * - Type-safe direction handling
 * - Modern container management for objects and actors
 * - Structured room flags and sector types
 * - JSON serialization/deserialization
 * - Exit system with door states and keys
 * - Light level and visibility calculations
 */

/**
 * Direction enum - use database-defined enum directly.
 * This ensures the game and database stay in sync automatically.
 */
using Direction = db::Direction;

/** Room sector types affecting movement and behavior */
enum class SectorType {
    Inside = 0,
    City,
    Field,
    Forest,
    Hills,
    Mountains,
    Water_Swim,
    Water_Noswim,
    Underwater,
    Flying,
    Desert,
    Swamp,
    Beach,
    Road,
    Underground,
    Lava,
    Ice,
    Astral,
    Fire,
    Lightning,
    Spirit,
    Badlands,
    Void,

    // Sentinel
    Undefined
};

// RoomFlag enum REMOVED - replaced by:
// - base_light_level_ field for lighting (numeric, more flexible)
// - Lua room restrictions for access control (godroom, no_mob, no_magic, etc.)
// - Sector type for environment (indoors, underwater, underground, etc.)
// - capacity_ field for occupancy limits
// - Mob presence for features (shop, bank, inn, temple)

/** Simple door state for scripting API */
struct DoorState {
    bool closed = false;
    bool locked = false;
    bool hidden = false;
};

/** Exit information with door states and access control */
struct ExitInfo {
    EntityId to_room = INVALID_ENTITY_ID;     // Destination room
    std::string description;                   // Exit description
    std::string keyword;                       // Door keyword

    // Door state
    bool has_door = false;
    bool is_closed = false;
    bool is_locked = false;
    bool is_hidden = false;
    bool is_pickproof = false;

    EntityId key_id = INVALID_ENTITY_ID;      // Key object ID
    int difficulty = 0;                        // Pick/bash difficulty

    /** Check if exit is passable */
    bool is_passable() const {
        return to_room.is_valid() && (!has_door || !is_closed);
    }

    /** Check if exit has functional door */
    bool has_functional_door() const {
        return has_door && !keyword.empty();
    }

    /** Get door state description */
    std::string door_state_description() const;

    /** JSON serialization */
    nlohmann::json to_json() const;
    static Result<ExitInfo> from_json(const nlohmann::json& json);
};

/** Room contents tracking */
struct RoomContents {
    std::vector<std::shared_ptr<Object>> objects;
    std::vector<std::shared_ptr<Actor>> actors;

    /** Add object to room */
    void add_object(std::shared_ptr<Object> obj);

    /** Remove object from room */
    bool remove_object(EntityId obj_id);

    /** Add actor to room */
    void add_actor(std::shared_ptr<Actor> actor);

    /** Remove actor from room */
    bool remove_actor(EntityId actor_id);

    /** Find object by ID */
    std::shared_ptr<Object> find_object(EntityId id) const;

    /** Find actor by ID */
    std::shared_ptr<Actor> find_actor(EntityId id) const;

    /** Get all objects matching keyword */
    std::vector<std::shared_ptr<Object>> find_objects_by_keyword(std::string_view keyword) const;

    /** Get all actors matching keyword */
    std::vector<std::shared_ptr<Actor>> find_actors_by_keyword(std::string_view keyword) const;

    /** Clear all contents */
    void clear();
};

/** Modern Room class */
class Room : public Entity {
public:
    /** Create room with ID, name, and sector type */
    static Result<std::unique_ptr<Room>> create(EntityId id, std::string_view name, SectorType sector = SectorType::Inside);

    /** Create room from JSON data */
    static Result<std::unique_ptr<Room>> from_json(const nlohmann::json& json);

    /** Destructor */
    virtual ~Room() = default;

    // Basic Properties
    SectorType sector_type() const { return sector_type_; }
    void set_sector_type(SectorType sector) { sector_type_ = sector; }

    EntityId zone_id() const { return zone_id_; }
    void set_zone_id(EntityId zone) { zone_id_ = zone; }

    // Base light level (from database)
    // Positive = lit, negative = dark, 0 = ambient (time/weather dependent)
    int base_light_level() const { return base_light_level_; }
    void set_base_light_level(int level) { base_light_level_ = level; }

    // Capacity for occupancy limits (from database)
    int capacity() const { return capacity_; }
    void set_capacity(int cap) { capacity_ = cap; }

    // Exit system
    bool has_exit(Direction dir) const;
    const ExitInfo* get_exit(Direction dir) const;
    ExitInfo* get_exit_mutable(Direction dir);

    Result<void> set_exit(Direction dir, const ExitInfo& exit);
    void remove_exit(Direction dir);

    std::vector<Direction> get_available_exits() const;
    std::vector<Direction> get_visible_exits(const Actor* observer = nullptr) const;

    // Door manipulation (for scripting API)
    Result<void> open_door(Direction dir);
    Result<void> close_door(Direction dir);
    Result<void> lock_door(Direction dir);
    Result<void> unlock_door(Direction dir);
    Result<void> set_door_state(Direction dir, const DoorState& state);

    // Contents management
    const RoomContents& contents() const { return contents_; }
    RoomContents& contents_mutable() { return contents_; }

    void add_object(std::shared_ptr<Object> obj);
    bool remove_object(EntityId obj_id);
    void add_actor(std::shared_ptr<Actor> actor);
    bool remove_actor(EntityId actor_id);

    std::shared_ptr<Object> find_object(EntityId id) const;
    std::shared_ptr<Actor> find_actor(EntityId id) const;

    std::vector<std::shared_ptr<Object>> find_objects_by_keyword(std::string_view keyword) const;
    std::vector<std::shared_ptr<Actor>> find_actors_by_keyword(std::string_view keyword) const;

    // Visibility and lighting
    bool is_dark() const;
    bool is_naturally_lit() const;
    int calculate_effective_light() const;
    bool can_see_in_room(const Actor* observer) const;

    // Room state properties (from database)
    bool is_peaceful() const { return is_peaceful_; }
    void set_peaceful(bool peaceful) { is_peaceful_ = peaceful; }

    bool allows_magic() const { return allows_magic_; }
    void set_allows_magic(bool allows) { allows_magic_ = allows; }

    bool allows_recall() const { return allows_recall_; }
    void set_allows_recall(bool allows) { allows_recall_ = allows; }

    bool allows_summon() const { return allows_summon_; }
    void set_allows_summon(bool allows) { allows_summon_ = allows; }

    bool allows_teleport() const { return allows_teleport_; }
    void set_allows_teleport(bool allows) { allows_teleport_ = allows; }

    bool is_death_trap() const { return is_death_trap_; }
    void set_death_trap(bool death_trap) { is_death_trap_ = death_trap; }

    // Entry restriction (Lua script for access control)
    const std::string& entry_restriction() const { return entry_restriction_; }
    void set_entry_restriction(std::string_view restriction) { entry_restriction_ = std::string(restriction); }
    bool has_entry_restriction() const { return !entry_restriction_.empty(); }

    // Capacity checks
    int max_occupants() const;
    bool can_accommodate(const Actor* actor) const;
    bool is_full() const;

    // Room descriptions
    std::string get_room_description(const Actor* observer = nullptr) const;
    std::string get_exits_description(const Actor* observer = nullptr) const;
    std::string get_contents_description(const Actor* observer = nullptr) const;

    // Serialization
    nlohmann::json to_json() const override;
    Result<void> validate() const override;

    /** Get comprehensive stat information for debugging/admin commands */
    std::string get_stat_info() const;

    // Layout coordinates for mapping (from database)
    std::optional<int> layout_x() const { return layout_x_; }
    std::optional<int> layout_y() const { return layout_y_; }
    std::optional<int> layout_z() const { return layout_z_; }

    void set_layout_coords(std::optional<int> x, std::optional<int> y, std::optional<int> z) {
        layout_x_ = x;
        layout_y_ = y;
        layout_z_ = z;
    }

protected:
    /** Protected constructor for factory pattern */
    Room(EntityId id, std::string_view name, SectorType sector);

private:
    SectorType sector_type_;
    int base_light_level_ = 0;   // Base light level from database
    int capacity_ = 10;          // Max occupants (0 = unlimited)
    EntityId zone_id_;
    std::unordered_map<Direction, ExitInfo> exits_;
    RoomContents contents_;

    // Room state flags (from database)
    bool is_peaceful_ = false;      // No combat allowed
    bool allows_magic_ = true;      // Magic can be cast
    bool allows_recall_ = true;     // Recall spell works
    bool allows_summon_ = true;     // Summon spell works
    bool allows_teleport_ = true;   // Teleport works
    bool is_death_trap_ = false;    // Instant death room

    // Entry restriction Lua script (from database)
    std::string entry_restriction_;

    // Layout coordinates for visual mapping
    std::optional<int> layout_x_;
    std::optional<int> layout_y_;
    std::optional<int> layout_z_;

    /** Update cached lighting calculations */
    void update_lighting_cache();

    /** Check if room naturally has light based on sector */
    bool sector_provides_light() const;
};

/** Utility functions for room management */
namespace RoomUtils {
    /** Get direction name */
    std::string_view get_direction_name(Direction dir);

    /** Parse direction from string */
    std::optional<Direction> parse_direction(std::string_view dir_name);

    /** Get opposite direction */
    Direction get_opposite_direction(Direction dir);

    /** Get direction abbreviation */
    std::string_view get_direction_abbrev(Direction dir);

    /** Get sector type name */
    std::string_view get_sector_name(SectorType sector);

    /** Parse sector type from string */
    std::optional<SectorType> parse_sector_type(std::string_view sector_name);

    /** Convert legacy numeric sector to SectorType enum */
    SectorType sector_from_number(int sector_num);

    // RoomFlag functions REMOVED - flags replaced by baseLightLevel and Lua restrictions

    /** Calculate movement cost for sector type */
    int get_movement_cost(SectorType sector);

    /** Check if sector allows flying */
    bool sector_allows_flying(SectorType sector);

    /** Check if sector is water */
    bool is_water_sector(SectorType sector);

    /** Check if sector requires swimming */
    bool requires_swimming(SectorType sector);

    /** Get natural light level for sector */
    int get_sector_light_level(SectorType sector);

    /** Get sector color code for display */
    std::string_view get_sector_color(SectorType sector);

    /** Check if sector is outdoors (affected by weather and sunlight) */
    bool is_outdoor_sector(SectorType sector);
}

/** Formatting support for Direction */
template<>
struct fmt::formatter<Direction> {
    constexpr auto parse(format_parse_context& ctx) {
        return ctx.begin();
    }

    template<typename FormatContext>
    auto format(const Direction& dir, FormatContext& ctx) const {
        return fmt::format_to(ctx.out(), "{}", RoomUtils::get_direction_name(dir));
    }
};

/** Formatting support for SectorType */
template<>
struct fmt::formatter<SectorType> {
    constexpr auto parse(format_parse_context& ctx) {
        return ctx.begin();
    }

    template<typename FormatContext>
    auto format(const SectorType& sector, FormatContext& ctx) const {
        return fmt::format_to(ctx.out(), "{}", RoomUtils::get_sector_name(sector));
    }
};

// RoomFlag formatter REMOVED - flags replaced by baseLightLevel and Lua restrictions
