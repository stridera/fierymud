#pragma once

#include "../core/entity.hpp"
#include "../core/ids.hpp"
#include "../core/result.hpp"

#include <chrono>
#include <fmt/format.h>
#include <functional>
#include <magic_enum/magic_enum.hpp>
#include <memory>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

// Forward declarations
class Room;
class Object;
class Actor;
class Mobile;
class Container;
class Logger;

/** Zone reset callback types */
using SpawnMobileCallback = std::function<std::shared_ptr<Mobile>(EntityId mobile_id, EntityId room_id)>;
using SpawnObjectCallback = std::function<std::shared_ptr<Object>(EntityId object_id, EntityId room_id)>;
using RemoveObjectCallback = std::function<bool(EntityId object_id, EntityId room_id)>;
using GetRoomCallback = std::function<std::shared_ptr<Room>(EntityId room_id)>;
using CleanupZoneMobilesCallback = std::function<void(EntityId zone_id)>;

/**
 * Modern zone system for FieryMUD.
 *
 * Replaces legacy zone structures with:
 * - JSON-based zone file loading
 * - Modern zone command processing
 * - Type-safe zone flags and reset modes
 * - Structured zone reset system
 * - Performance-optimized room/object/mobile management
 */

/** Zone reset modes affecting how frequently zones reset */
enum class ResetMode {
    Never = 0, // Zone never resets
    Empty,     // Reset when no players present
    Always,    // Reset on schedule regardless of players
    OnReboot,  // Reset only on server reboot
    Manual     // Reset only via admin command
};

/** Zone flags affecting behavior */
enum class ZoneFlag {
    Closed = 0,  // Zone is closed to players
    NoMortals,   // Only immortals can enter
    Quest,       // Quest zone
    Grid,        // Grid-based movement
    Maze,        // Maze zone
    Recall_Ok,   // Allow recall in zone
    Summon_Ok,   // Allow summoning in zone
    Teleport_Ok, // Allow teleporting in zone
    Search,      // Zone allows searching
    Noattack,    // No combat allowed
    Worldmap,    // Zone is part of world map
    Astral,      // Astral plane zone

    // Clan zones
    ClanZone, // Zone belongs to a clan

    // Special zones
    Newbie, // Newbie zone
    Arena,  // Arena zone
    Prison, // Prison zone

    // Weather/environment
    NoWeather,   // Zone not affected by weather
    Underground, // Underground zone

    // PK zones
    ChaosOk, // PK allowed
    LawfulOk // Only lawful PK allowed
};

/** Zone command types for automated zone resets */
enum class ZoneCommandType {
    // Mobile commands
    Load_Mobile = 0, // Load mobile in room
    Follow_Mobile,   // Make mobile follow another

    // Object commands
    Load_Object,  // Load object in room (with hierarchical contents)
    Give_Object,  // Give object to mobile
    Equip_Object, // Equip object on mobile

    // Door commands
    Open_Door,   // Open door
    Close_Door,  // Close door
    Lock_Door,   // Lock door
    Unlock_Door, // Unlock door

    // Trigger commands
    Trigger, // Execute trigger

    // Control commands
    Comment, // Comment line
    Wait,    // Wait N seconds
    Random,  // Random chance command

    // Room commands
    Teleport, // Teleport mobile to room
    Force,    // Force mobile to do command

    // Special commands
    Remove_Object, // Remove object from game
    Halt           // Stop processing commands
};

/** Nested object content for hierarchical spawning */
struct ObjectContent {
    EntityId object_id = INVALID_ENTITY_ID; // Object prototype ID
    int quantity = 1;                       // Number to spawn
    std::string comment;                    // Optional comment
    std::vector<ObjectContent> contents;    // Nested contents (arbitrary depth)
};

/** Zone command structure */
struct ZoneCommand {
    ZoneCommandType command_type;
    int if_flag; // Condition flag

    // Meaningful command parameters based on command type
    EntityId entity_id = INVALID_ENTITY_ID;    // ID of mobile/object to load
    EntityId room_id = INVALID_ENTITY_ID;      // Target room ID
    EntityId container_id = INVALID_ENTITY_ID; // Container/mobile ID for Give/Put commands
    int max_count = 1;                         // Maximum number to spawn
    int reset_group = 0;                       // Links equipment to its Load_Mobile command

    std::string comment; // Comment for documentation

    // Hierarchical object contents (for Load_Object commands with containers)
    std::vector<ObjectContent> contents;

    /** Check if command should execute based on conditions */
    bool should_execute() const;

    /** Get command description for debugging */
    std::string to_string() const;

    /** JSON serialization */
    nlohmann::json to_json() const;
};

/** Zone reset statistics */
struct ZoneStats {
    std::chrono::steady_clock::time_point last_reset;
    std::chrono::steady_clock::time_point creation_time;
    int reset_count = 0;
    int player_count = 0;
    int mobile_count = 0;
    int object_count = 0;

    /** Get uptime in seconds */
    std::chrono::seconds uptime() const {
        return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - creation_time);
    }

    /** Get time since last reset */
    std::chrono::seconds time_since_reset() const {
        return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - last_reset);
    }
};

/** Modern Zone class */
class Zone : public Entity {
  public:
    /** Create zone with ID, name, and reset parameters */
    static Result<std::unique_ptr<Zone>> create(EntityId id, std::string_view name, int reset_minutes = 30,
                                                ResetMode mode = ResetMode::Empty);

    /** Create zone from JSON file */
    static Result<std::unique_ptr<Zone>> from_json_file(const std::string &filename);

    /** Create zone from JSON data */
    static Result<std::unique_ptr<Zone>> from_json(const nlohmann::json &json);

    /** Destructor */
    virtual ~Zone() = default;

    // Basic Properties
    int reset_minutes() const { return reset_minutes_; }
    void set_reset_minutes(int minutes) { reset_minutes_ = std::max(0, minutes); }

    ResetMode reset_mode() const { return reset_mode_; }
    void set_reset_mode(ResetMode mode) { reset_mode_ = mode; }

    int min_level() const { return min_level_; }
    void set_min_level(int level) { min_level_ = std::max(0, level); }

    int max_level() const { return max_level_; }
    void set_max_level(int level) { max_level_ = std::max(min_level_, level); }

    std::string_view builders() const { return builders_; }
    void set_builders(std::string_view builders) { builders_ = builders; }

    // Flag management
    bool has_flag(ZoneFlag flag) const;
    void set_flag(ZoneFlag flag, bool value = true);
    void remove_flag(ZoneFlag flag) { set_flag(flag, false); }
    const std::unordered_set<ZoneFlag> &flags() const { return flags_; }

    // Room management
    void add_room(EntityId room_id);
    void remove_room(EntityId room_id);
    bool contains_room(EntityId room_id) const;
    const std::unordered_set<EntityId> &rooms() const { return rooms_; }

    EntityId first_room() const { return first_room_; }
    void set_first_room(EntityId room_id) { first_room_ = room_id; }

    EntityId last_room() const { return last_room_; }
    void set_last_room(EntityId room_id) { last_room_ = room_id; }

    // Zone commands
    void add_command(const ZoneCommand &cmd);
    void clear_commands() { commands_.clear(); }
    const std::vector<ZoneCommand> &commands() const { return commands_; }

    // Reset system
    bool needs_reset() const;
    void force_reset();
    Result<void> execute_reset();

    void set_reset_time(std::chrono::steady_clock::time_point time) { stats_.last_reset = time; }

    // Reset callbacks
    void set_spawn_mobile_callback(SpawnMobileCallback callback) { spawn_mobile_callback_ = std::move(callback); }
    void set_spawn_object_callback(SpawnObjectCallback callback) { spawn_object_callback_ = std::move(callback); }
    void set_remove_object_callback(RemoveObjectCallback callback) { remove_object_callback_ = std::move(callback); }
    void set_get_room_callback(GetRoomCallback callback) { get_room_callback_ = std::move(callback); }
    void set_cleanup_zone_mobiles_callback(CleanupZoneMobilesCallback callback) {
        cleanup_zone_mobiles_callback_ = std::move(callback);
    }

    // Statistics
    const ZoneStats &stats() const { return stats_; }
    ZoneStats &stats_mutable() { return stats_; }

    void increment_reset_count() { stats_.reset_count++; }
    void set_player_count(int count) { stats_.player_count = std::max(0, count); }
    void set_mobile_count(int count) { stats_.mobile_count = std::max(0, count); }
    void set_object_count(int count) { stats_.object_count = std::max(0, count); }

    // Zone state queries
    bool is_closed() const { return has_flag(ZoneFlag::Closed); }
    bool allows_mortals() const { return !has_flag(ZoneFlag::NoMortals); }
    bool is_quest_zone() const { return has_flag(ZoneFlag::Quest); }
    bool allows_recall() const { return true; } // Always allow recall unless specifically forbidden
    bool allows_summon() const { return has_flag(ZoneFlag::Summon_Ok); }
    bool allows_teleport() const { return has_flag(ZoneFlag::Teleport_Ok); }
    bool allows_combat() const { return !has_flag(ZoneFlag::Noattack); }
    bool is_pk_zone() const { return has_flag(ZoneFlag::ChaosOk) || has_flag(ZoneFlag::LawfulOk); }

    // Level restrictions
    bool allows_level(int level) const { return level >= min_level_ && level <= max_level_; }

    // Weather support
    bool has_weather_override() const { return has_flag(ZoneFlag::NoWeather); }
    bool is_underground() const { return has_flag(ZoneFlag::Underground); }

    // Zone file operations
    Result<void> save_to_file(const std::string &filename) const;
    Result<void> reload_from_file(const std::string &filename);

    // Serialization
    nlohmann::json to_json() const override;
    Result<void> validate() const override;

  protected:
    /** Protected constructor for factory pattern */
    Zone(EntityId id, std::string_view name, int reset_minutes, ResetMode mode);

  private:
    int reset_minutes_;
    ResetMode reset_mode_;
    int min_level_;
    int max_level_;
    std::string builders_;
    std::unordered_set<ZoneFlag> flags_;
    std::unordered_set<EntityId> rooms_;
    EntityId first_room_;
    EntityId last_room_;
    std::vector<ZoneCommand> commands_;
    ZoneStats stats_;

    // Reset callbacks
    SpawnMobileCallback spawn_mobile_callback_;
    SpawnObjectCallback spawn_object_callback_;
    RemoveObjectCallback remove_object_callback_;
    GetRoomCallback get_room_callback_;
    CleanupZoneMobilesCallback cleanup_zone_mobiles_callback_;

    /** Execute a single zone command */
    Result<bool> execute_command(const ZoneCommand &cmd);

    /** Zone command implementations */
    Result<bool> execute_load_mobile(const ZoneCommand &cmd);
    Result<bool> execute_load_object(const ZoneCommand &cmd);
    Result<bool> execute_give_object(const ZoneCommand &cmd);
    Result<bool> execute_equip_object(const ZoneCommand &cmd);
    Result<bool> execute_remove_object(const ZoneCommand &cmd);
    Result<bool> execute_door_command(const ZoneCommand &cmd);

    /** Process equipment and inventory for a specific mobile instance */
    void process_mobile_equipment(std::shared_ptr<Mobile> mobile, EntityId mobile_id, int reset_group);

    /** Recursively spawn nested objects into a container */
    void spawn_contents_recursive(Container *container, const std::vector<ObjectContent> &contents,
                                  std::shared_ptr<Logger> logger);

    /** Check if zone is empty of players */
    bool is_empty_of_players() const;

    /** Update statistics */
    void update_statistics();

    /** Parse nested zone command format from JSON */
    static Result<void> parse_nested_zone_commands(const nlohmann::json &commands_json, Zone *zone);
};

/** Zone command parser for loading from legacy format */
class ZoneCommandParser {
  public:
    /** Parse zone command from text line */
    static Result<ZoneCommand> parse_command_line(std::string_view line);

    /** Parse multiple commands from text */
    static Result<std::vector<ZoneCommand>> parse_commands(std::string_view text);

    /** Convert command back to text format */
    static std::string format_command(const ZoneCommand &cmd);

  private:
    /** Parse command arguments */
    static Result<ZoneCommand> parse_args(ZoneCommandType type, const std::vector<std::string> &args,
                                          std::string_view original_line);
};

/** Utility functions for zone management */
namespace ZoneUtils {
/** Get reset mode name */
std::string_view get_reset_mode_name(ResetMode mode);

/** Parse reset mode from string */
std::optional<ResetMode> parse_reset_mode(std::string_view mode_name);

/** Get zone flag name */
std::string_view get_flag_name(ZoneFlag flag);

/** Parse zone flag from string */
std::optional<ZoneFlag> parse_zone_flag(std::string_view flag_name);

/** Get command type name */
std::string_view get_command_type_name(ZoneCommandType type);

/** Parse command type from string */
std::optional<ZoneCommandType> parse_command_type(std::string_view type_name);

/** Get command type from character (legacy format) */
std::optional<ZoneCommandType> get_command_type_from_char(char cmd_char);

/** Get character from command type (legacy format) */
char get_char_from_command_type(ZoneCommandType type);

/** Check if zone file exists */
bool zone_file_exists(const std::string &filename);

/** Get default zone file path */
std::string get_zone_file_path(int zone_number);

/** Extract zone number from filename */
std::optional<int> extract_zone_number(const std::string &filename);

/** Validate zone file format */
Result<void> validate_zone_file(const std::string &filename);
} // namespace ZoneUtils

/** Formatting support for ResetMode */
template <> struct fmt::formatter<ResetMode> {
    constexpr auto parse(format_parse_context &ctx) { return ctx.begin(); }

    template <typename FormatContext> auto format(const ResetMode &mode, FormatContext &ctx) const {
        return fmt::format_to(ctx.out(), "{}", ZoneUtils::get_reset_mode_name(mode));
    }
};

/** Formatting support for ZoneFlag */
template <> struct fmt::formatter<ZoneFlag> {
    constexpr auto parse(format_parse_context &ctx) { return ctx.begin(); }

    template <typename FormatContext> auto format(const ZoneFlag &flag, FormatContext &ctx) const {
        return fmt::format_to(ctx.out(), "{}", ZoneUtils::get_flag_name(flag));
    }
};

/** Formatting support for ZoneCommandType */
template <> struct fmt::formatter<ZoneCommandType> {
    constexpr auto parse(format_parse_context &ctx) { return ctx.begin(); }

    template <typename FormatContext> auto format(const ZoneCommandType &type, FormatContext &ctx) const {
        return fmt::format_to(ctx.out(), "{}", ZoneUtils::get_command_type_name(type));
    }
};
