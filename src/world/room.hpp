#pragma once

#include "../core/entity.hpp"
#include "../core/result.hpp"
#include "../core/ids.hpp"

#include <memory>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <string_view>
#include <optional>
#include <magic_enum/magic_enum.hpp>
#include <nlohmann/json.hpp>

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

/** Cardinal and special directions */
enum class Direction {
    North = 0,
    East,
    South, 
    West,
    Up,
    Down,
    Northeast,
    Northwest,
    Southeast,
    Southwest,
    
    // Special exits
    In,
    Out,
    Portal,
    
    // Sentinel
    None
};

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

/** Room flags affecting behavior and access */
enum class RoomFlag {
    // Core flags (from legacy CircleMUD)
    Dark = 0,           // Always dark
    Death,              // Death trap
    NoMob,              // Mobiles cannot enter
    Indoors,            // Indoor room
    Peaceful,           // No combat allowed
    Soundproof,         // Sound doesn't carry
    NoTrack,            // Cannot be tracked to/from
    NoMagic,            // No magic allowed
    Tunnel,             // Only one person at a time
    Private,            // Private room (2 people max)
    Godroom,            // Gods only
    House,              // Player house
    HouseCrash,         // House save on crash
    Atrium,             // Multi-person private
    OLC,                // OLC room
    BFS_Mark,           // Breadth-First Search mark
    Vehicle,            // Vehicle room
    Underground,        // Underground room
    Current,            // Has water current
    Timed_DT,           // Timed death trap
    Earth,              // Earth-aligned room
    Air,                // Air-aligned room
    Fire,               // Fire-aligned room
    Water,              // Water-aligned room
    NoRecall,           // Cannot recall from/to
    NoSummon,           // Cannot summon to/from
    NoLocate,           // Cannot locate in
    NoTeleport,         // Cannot teleport to/from
    AlwaysLit,          // Always lit (never dark)

    // Clan system flags
    ClanEntrance,       // Clan entrance room
    ClanStorage,        // Clan storage room

    // Special flags
    Arena,              // Arena room
    Shop,               // Shop room
    Temple,             // Temple/holy place - player rises from meditation on return
    Bank,               // Bank room
    Inn,                // Rentable room - player checks out on return
    Campsite,           // Designated camping area - player breaks camp on return

    // Map and navigation flags
    Worldmap,           // Part of world map
    FerryDest,          // Ferry destination
    Isolated,           // Isolated from main world
    AltExit,            // Has alternative exit display
    Observatory,        // Observatory room

    // Room size flags (combat/capacity)
    Large,              // Large room
    MediumLarge,        // Medium-large room
    Medium,             // Medium room
    MediumSmall,        // Medium-small room
    Small,              // Small room
    VerySmall,          // Very small room
    OnePerson,          // Single occupant only

    // Special ability flags
    Guildhall,          // Guild hall room
    NoWell,             // Cannot use well of souls
    NoScan,             // Cannot scan in room
    Underdark,          // Underdark area
    NoShift,            // Cannot plane shift
    EffectsNext         // Effects carry to next room
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
    
    int light_level() const { return light_level_; }
    void set_light_level(int level) { light_level_ = std::max(0, level); }
    
    EntityId zone_id() const { return zone_id_; }
    void set_zone_id(EntityId zone) { zone_id_ = zone; }
    
    // Flag management
    bool has_flag(RoomFlag flag) const;
    void set_flag(RoomFlag flag, bool value = true);
    void remove_flag(RoomFlag flag) { set_flag(flag, false); }
    const std::unordered_set<RoomFlag>& flags() const { return flags_; }
    
    // Exit system
    bool has_exit(Direction dir) const;
    const ExitInfo* get_exit(Direction dir) const;
    ExitInfo* get_exit_mutable(Direction dir);
    
    Result<void> set_exit(Direction dir, const ExitInfo& exit);
    void remove_exit(Direction dir);
    
    std::vector<Direction> get_available_exits() const;
    std::vector<Direction> get_visible_exits(const Actor* observer = nullptr) const;
    
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
    
    // Room state queries
    bool is_peaceful() const { return has_flag(RoomFlag::Peaceful); }
    bool allows_magic() const { return !has_flag(RoomFlag::NoMagic); }
    bool allows_recall() const { return !has_flag(RoomFlag::NoRecall); }
    bool allows_summon() const { return !has_flag(RoomFlag::NoSummon); }
    bool allows_teleport() const { return !has_flag(RoomFlag::NoTeleport); }
    bool is_private() const { return has_flag(RoomFlag::Private); }
    bool is_death_trap() const { return has_flag(RoomFlag::Death); }
    
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
    
protected:
    /** Protected constructor for factory pattern */
    Room(EntityId id, std::string_view name, SectorType sector);
    
private:
    SectorType sector_type_;
    int light_level_;
    EntityId zone_id_;
    std::unordered_set<RoomFlag> flags_;
    std::unordered_map<Direction, ExitInfo> exits_;
    RoomContents contents_;
    
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

    /** Get room flag name */
    std::string_view get_flag_name(RoomFlag flag);
    
    /** Parse room flag from string */
    std::optional<RoomFlag> parse_room_flag(std::string_view flag_name);
    
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

/** Formatting support for RoomFlag */
template<>
struct fmt::formatter<RoomFlag> {
    constexpr auto parse(format_parse_context& ctx) {
        return ctx.begin();
    }
    
    template<typename FormatContext>
    auto format(const RoomFlag& flag, FormatContext& ctx) const {
        return fmt::format_to(ctx.out(), "{}", RoomUtils::get_flag_name(flag));
    }
};