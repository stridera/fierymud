#pragma once

#include "core/result.hpp"
#include "core/ids.hpp"
#include "world/zone.hpp"
#include "world/room.hpp"
#include "core/actor.hpp"
#include "core/object.hpp"
#include <pqxx/pqxx>
#include <vector>
#include <memory>
#include <chrono>
#include <optional>

/**
 * World data query layer for PostgreSQL database.
 *
 * Provides SQL queries for loading zones, rooms, mobs, and objects
 * using composite primary keys (zone_id, local_id).
 *
 * All queries use prepared statements for security and performance.
 * Composite key format: WHERE "zoneId" = $1 AND id = $2
 */
namespace WorldQueries {

// Zone queries
Result<std::vector<std::unique_ptr<Zone>>> load_all_zones(pqxx::work& txn);
Result<std::unique_ptr<Zone>> load_zone(pqxx::work& txn, int zone_id);

// Room queries
Result<std::vector<std::unique_ptr<Room>>> load_rooms_in_zone(
    pqxx::work& txn, int zone_id);
Result<std::unique_ptr<Room>> load_room(
    pqxx::work& txn, int zone_id, int room_local_id);

// Exit queries (for rooms)
struct ExitData {
    Direction direction;
    EntityId to_room;
    std::string description;
    std::vector<std::string> keywords;
    EntityId key;
    std::vector<std::string> flags;
};
Result<std::vector<ExitData>> load_room_exits(
    pqxx::work& txn, int zone_id, int room_local_id);
Result<std::vector<ExitData>> load_all_exits_in_zone(
    pqxx::work& txn, int zone_id);

// Mobile Reset data structure
struct MobResetData {
    int id;                     // Reset ID (for equipment linkage)
    EntityId mob_id;            // Mob prototype to spawn (zone_id, mob_local_id)
    EntityId room_id;           // Room to spawn in (zone_id, room_local_id)
    int max_instances;          // Maximum instances allowed
    float probability;          // Spawn probability (0.0-1.0)
    std::string comment;        // Optional comment
};

// Mob equipment data (what a mob spawns wearing/carrying)
struct MobEquipmentData {
    int reset_id;               // Which mob reset this belongs to
    EntityId object_id;         // Object prototype (zone_id, obj_local_id)
    std::string wear_location;  // Where to equip (WIELD, HEAD, etc.) - empty for inventory
    int max_instances;          // Maximum instances
    float probability;          // Spawn probability
};

// Object Reset data structure
struct ObjectResetData {
    int id;                     // Reset ID
    EntityId object_id;         // Object prototype (zone_id, obj_local_id)
    EntityId room_id;           // Room to place in (zone_id, room_local_id) - may be invalid for containers
    int container_reset_id;     // If > 0, place inside this container reset
    int max_instances;          // Maximum instances
    float probability;          // Spawn probability
    std::string comment;        // Optional comment
};

// Reset queries
Result<std::vector<MobResetData>> load_mob_resets_in_zone(
    pqxx::work& txn, int zone_id);
Result<std::vector<MobEquipmentData>> load_mob_equipment_for_reset(
    pqxx::work& txn, int reset_id);
Result<std::vector<MobEquipmentData>> load_all_mob_equipment_in_zone(
    pqxx::work& txn, int zone_id);
Result<std::vector<ObjectResetData>> load_object_resets_in_zone(
    pqxx::work& txn, int zone_id);

// Mobile queries
Result<std::vector<std::unique_ptr<Mobile>>> load_mobs_in_zone(
    pqxx::work& txn, int zone_id);
Result<std::unique_ptr<Mobile>> load_mob(
    pqxx::work& txn, int zone_id, int mob_local_id);

// Object queries
Result<std::vector<std::unique_ptr<Object>>> load_objects_in_zone(
    pqxx::work& txn, int zone_id);
Result<std::unique_ptr<Object>> load_object(
    pqxx::work& txn, int zone_id, int object_local_id);

// ============================================================================
// Ability/Skill System Queries
// ============================================================================

/** Ability type enumeration matching database values */
enum class AbilityType {
    Spell,
    Skill,
    Chant,
    Song
};

/** Core ability data from database */
struct AbilityData {
    int id;
    std::string name;           // Display name (may have color codes)
    std::string plain_name;     // Plain text name for lookups
    std::string description;
    AbilityType type;
    int min_position;           // Minimum position to use (0=dead, 4=standing, etc.)
    bool violent;               // Is this a violent/combat ability?
    int cast_time_rounds;       // Casting time in rounds
    int cooldown_ms;            // Cooldown in milliseconds
    bool is_area;               // Area effect?
    bool is_toggle;             // Toggle ability (on/off state like sneak, meditate)
    bool combat_ok;             // Can be used during combat
    bool in_combat_only;        // Can ONLY be used during combat
    std::string sphere;         // Spell sphere (fire, water, healing, etc.)
    std::string damage_type;    // Primary damage type
    int pages;                  // Spellbook pages required
    int memorization_time;      // Additional memorization rounds
    bool quest_only;            // Quest-restricted ability
    bool humanoid_only;         // Only available to humanoids
};

/** Class-specific ability availability */
struct ClassAbilityData {
    int ability_id;
    int class_id;
    int min_level;              // Minimum level to learn
};

/** Character's learned ability */
struct CharacterAbilityData {
    std::string character_id;
    int ability_id;
    bool known;                 // Has the character learned this?
    int proficiency;            // 0-100 skill percentage
    std::optional<std::chrono::system_clock::time_point> last_used;
};

// Load all abilities from database
Result<std::vector<AbilityData>> load_all_abilities(pqxx::work& txn);

// Load abilities available to a specific class
Result<std::vector<ClassAbilityData>> load_class_abilities(
    pqxx::work& txn, int class_id);

// Load abilities a character has learned
Result<std::vector<CharacterAbilityData>> load_character_abilities(
    pqxx::work& txn, const std::string& character_id);

/** Character ability with full metadata (for efficient loading) */
struct CharacterAbilityWithMetadata {
    int ability_id;
    std::string name;           // Ability display name
    std::string plain_name;     // Plain name for lookups (e.g., "LIGHTNING_BOLT")
    std::string description;    // Description for help system
    bool known;
    int proficiency;            // 0-1000 proficiency (10x of percentage)
    AbilityType type;           // SKILL, SPELL, etc.
    bool violent;               // Is this a combat ability?
};

// Load character abilities with full metadata (single efficient query)
Result<std::vector<CharacterAbilityWithMetadata>> load_character_abilities_with_metadata(
    pqxx::work& txn, const std::string& character_id);

// Save/update a character's ability
Result<void> save_character_ability(
    pqxx::work& txn,
    const std::string& character_id,
    int ability_id,
    bool known,
    int proficiency);

// Load a single ability by ID
Result<AbilityData> load_ability(pqxx::work& txn, int ability_id);

// Load a single ability by plain name
Result<AbilityData> load_ability_by_name(pqxx::work& txn, const std::string& name);

// =============================================================================
// Effect System Data Structures
// =============================================================================

/** Effect definition from the Effect table */
struct EffectData {
    int id;
    std::string name;
    std::string description;
    std::string effect_type;        // "damage", "heal", "interrupt", etc.
    std::string default_params;     // JSON string with default parameters
};

/** Ability-to-Effect linking from AbilityEffect table */
struct AbilityEffectData {
    int ability_id;
    int effect_id;
    std::string override_params;    // JSON string with ability-specific overrides
    int order;                      // Execution order
    std::string trigger;            // "on_hit", "on_cast", "periodic", etc.
    int chance_percent;             // Chance to apply (0-100)
    std::string condition;          // Optional Lua condition
};

/** Custom messages for abilities from AbilityMessages table */
struct AbilityMessagesData {
    int ability_id;
    std::string start_to_caster;    // Message to caster when starting
    std::string start_to_victim;    // Message to victim when starting
    std::string start_to_room;      // Message to room when starting
    std::string success_to_caster;  // Message to caster on success
    std::string success_to_victim;  // Message to victim on success
    std::string success_to_room;    // Message to room on success
    std::string success_to_self;    // Message to caster when targeting self
    std::string success_self_room;  // Room message when caster targets self
    std::string fail_to_caster;     // Message to caster on failure
    std::string fail_to_victim;     // Message to victim on failure
    std::string fail_to_room;       // Message to room on failure
    std::string wearoff_to_target;  // Message when effect wears off
    std::string wearoff_to_room;    // Room message when effect wears off
    std::string look_message;       // Message when looking at someone affected
};

/** Restriction requirements for abilities from AbilityRestrictions table */
struct AbilityRestrictionsData {
    int ability_id;
    std::string requirements_json;       // JSON array of requirement objects
    std::string custom_requirement_lua;  // Optional Lua script for custom checks
};

/** Single requirement parsed from requirements_json */
struct AbilityRequirement {
    std::string type;          // "hidden", "weapon_type", "position", "has_effect", etc.
    std::string value;         // The required value (e.g., "piercing", "standing")
    bool negated = false;      // If true, requirement is inverted (must NOT have)
};

/** Damage component for multi-element abilities from AbilityDamageComponent table */
struct AbilityDamageComponentData {
    int ability_id;
    std::string element;       // Element type (FIRE, COLD, LIGHTNING, etc.)
    std::string damage_formula; // Damage formula (e.g., "8d6", "2d6+level")
    int percentage;            // Percentage of total damage (should sum to 100)
    int sequence;              // Order of application
};

/** Load all effect definitions */
Result<std::vector<EffectData>> load_all_effects(pqxx::work& txn);

/** Load a single effect by ID */
Result<EffectData> load_effect(pqxx::work& txn, int effect_id);

/** Load all effects for a specific ability */
Result<std::vector<AbilityEffectData>> load_ability_effects(
    pqxx::work& txn, int ability_id);

/** Load effects for multiple abilities (batch load for efficiency) */
Result<std::vector<AbilityEffectData>> load_abilities_effects(
    pqxx::work& txn, const std::vector<int>& ability_ids);

/** Load messages for all abilities (batch load) */
Result<std::vector<AbilityMessagesData>> load_all_ability_messages(pqxx::work& txn);

/** Load messages for a single ability */
Result<std::optional<AbilityMessagesData>> load_ability_messages(
    pqxx::work& txn, int ability_id);

/** Load restrictions for all abilities (batch load) */
Result<std::vector<AbilityRestrictionsData>> load_all_ability_restrictions(pqxx::work& txn);

/** Load restrictions for a single ability */
Result<std::optional<AbilityRestrictionsData>> load_ability_restrictions(
    pqxx::work& txn, int ability_id);

/** Load damage components for all abilities (batch load) */
Result<std::vector<AbilityDamageComponentData>> load_all_ability_damage_components(pqxx::work& txn);

/** Load damage components for a single ability */
Result<std::vector<AbilityDamageComponentData>> load_ability_damage_components(
    pqxx::work& txn, int ability_id);

/** Parse requirements JSON into structured requirements.
 *  @param json_str The JSON string to parse
 *  @param ability_id Optional ability ID for better error messages
 */
std::vector<AbilityRequirement> parse_ability_requirements(
    std::string_view json_str, int ability_id = -1);

// =============================================================================
// Character/Player System Queries
// =============================================================================

/** Character data from the Characters table */
struct CharacterData {
    std::string id;                 // UUID
    std::string name;
    int level = 1;
    int alignment = 0;

    // Stats
    int strength = 13;
    int intelligence = 13;
    int wisdom = 13;
    int dexterity = 13;
    int constitution = 13;
    int charisma = 13;
    int luck = 13;

    // Vitals
    int hit_points = 100;
    int hit_points_max = 100;
    int movement = 100;
    int movement_max = 100;

    // Currency (copper/silver/gold/platinum)
    int copper = 0;
    int silver = 0;
    int gold = 0;
    int platinum = 0;
    int bank_copper = 0;
    int bank_silver = 0;
    int bank_gold = 0;
    int bank_platinum = 0;

    // Character info
    std::string password_hash;
    std::string race_type;          // "human", "elf", etc.
    std::string gender;             // "male", "female", "neutral"
    std::string player_class;       // "Warrior", "Cleric", etc.
    int class_id = 0;

    // Location (composite keys: zone_id + room_id)
    std::optional<int> current_room_zone_id;
    std::optional<int> current_room_id;
    std::optional<int> save_room_zone_id;
    std::optional<int> save_room_id;
    std::optional<int> home_room_zone_id;
    std::optional<int> home_room_id;

    // Combat stats
    int hit_roll = 0;
    int damage_roll = 0;
    int armor_class = 10;

    // Time/Session
    std::optional<std::chrono::system_clock::time_point> last_login;
    int time_played = 0;            // Seconds played
    bool is_online = false;

    // Needs (for survival mechanics)
    int hunger = 0;
    int thirst = 0;

    // Description/Title
    std::string description;
    std::string title;

    // Preferences
    std::string prompt = "<%%h/%%Hhp %%v/%%Vmv>";
    int page_length = 25;
    int wimpy_threshold = 0;

    // Flags (stored as arrays in database)
    std::vector<std::string> player_flags;
    std::vector<std::string> effect_flags;
    std::vector<std::string> privilege_flags;

    // Experience/Skills
    int experience = 0;
    int skill_points = 0;

    // Creation time
    std::chrono::system_clock::time_point created_at;
    std::chrono::system_clock::time_point updated_at;
};

/** Check if a character exists by name */
Result<bool> character_exists(pqxx::work& txn, const std::string& name);

/** Load character by name */
Result<CharacterData> load_character_by_name(pqxx::work& txn, const std::string& name);

/** Load character by ID */
Result<CharacterData> load_character_by_id(pqxx::work& txn, const std::string& id);

/** Verify character password */
Result<bool> verify_character_password(
    pqxx::work& txn,
    const std::string& name,
    const std::string& password);

/** Create a new character */
Result<CharacterData> create_character(
    pqxx::work& txn,
    const std::string& name,
    const std::string& password,
    const std::string& player_class,
    const std::string& race);

/** Save character data */
Result<void> save_character(pqxx::work& txn, const CharacterData& character);

/** Update character online status */
Result<void> set_character_online(
    pqxx::work& txn,
    const std::string& character_id,
    bool is_online);

/** Update character's last login time */
Result<void> update_last_login(pqxx::work& txn, const std::string& character_id);

/** Update character's location (using composite zone_id + room_id) */
Result<void> save_character_location(
    pqxx::work& txn,
    const std::string& character_id,
    int zone_id,
    int room_id);

// =============================================================================
// User/Account System Queries
// =============================================================================

/** User role enumeration matching database values */
enum class UserRole {
    Player,
    Immortal,
    Builder,
    HeadBuilder,
    Coder,
    God
};

/** User account data from the Users table */
struct UserData {
    std::string id;                 // UUID
    std::string email;
    std::string username;
    std::string password_hash;
    UserRole role = UserRole::Player;
    int failed_login_attempts = 0;
    std::optional<std::chrono::system_clock::time_point> locked_until;
    std::optional<std::chrono::system_clock::time_point> last_login_at;
    std::chrono::system_clock::time_point created_at;
    std::chrono::system_clock::time_point updated_at;
};

/** Summary of a character for selection menus */
struct CharacterSummary {
    std::string id;
    std::string name;
    int level;
    std::string player_class;
    std::string race_type;
    bool is_online;
    std::optional<std::chrono::system_clock::time_point> last_login;
};

/** Check if a user account exists by username or email */
Result<bool> user_exists(pqxx::work& txn, const std::string& username_or_email);

/** Load user by username */
Result<UserData> load_user_by_username(pqxx::work& txn, const std::string& username);

/** Load user by email */
Result<UserData> load_user_by_email(pqxx::work& txn, const std::string& email);

/** Load user by ID */
Result<UserData> load_user_by_id(pqxx::work& txn, const std::string& id);

/** Verify user password (with automatic hash upgrade for legacy passwords) */
Result<bool> verify_user_password(
    pqxx::work& txn,
    const std::string& username_or_email,
    const std::string& password);

/** Get all characters associated with a user account */
Result<std::vector<CharacterSummary>> get_user_characters(
    pqxx::work& txn,
    const std::string& user_id);

/** Update user's last login time and reset failed attempts */
Result<void> update_user_last_login(pqxx::work& txn, const std::string& user_id);

/** Increment failed login attempts for a user */
Result<void> increment_failed_login(pqxx::work& txn, const std::string& user_id);

/** Lock a user account until a specified time */
Result<void> lock_user_account(
    pqxx::work& txn,
    const std::string& user_id,
    std::chrono::system_clock::time_point until);

/** Link a character to a user account */
Result<void> link_character_to_user(
    pqxx::work& txn,
    const std::string& character_id,
    const std::string& user_id);

// =============================================================================
// Shop System Queries
// =============================================================================

/** Shop data from the Shops table */
struct ShopData {
    int id;
    EntityId keeper_id;             // Mob that runs this shop (zone_id, local_id)
    float buy_profit = 1.0f;        // Rate shopkeeper charges (1.0 = no markup)
    float sell_profit = 1.0f;       // Rate shopkeeper pays for items (1.0 = full price)
    int temper = 0;                 // Shopkeeper's temperament
    std::vector<std::string> no_such_item_messages;
    std::vector<std::string> do_not_buy_messages;
    std::vector<std::string> missing_cash_messages;
    std::vector<std::string> buy_messages;
    std::vector<std::string> sell_messages;
    std::vector<std::string> flags;
    std::vector<std::string> trades_with_flags;
};

/** Shop item data from the ShopItems table */
struct ShopItemData {
    EntityId object_id;             // Object prototype (zone_id, local_id)
    int amount = -1;                // Stock quantity (-1 = unlimited)
};

/** Load all shops from the database */
Result<std::vector<ShopData>> load_all_shops(pqxx::work& txn);

/** Load shops for a specific zone */
Result<std::vector<ShopData>> load_shops_in_zone(pqxx::work& txn, int zone_id);

/** Load items sold by a specific shop */
Result<std::vector<ShopItemData>> load_shop_items(
    pqxx::work& txn, int shop_zone_id, int shop_id);

// =============================================================================
// Character Item Queries
// =============================================================================

/** Character item data from the CharacterItems table */
struct CharacterItemData {
    int id;                         // Auto-increment primary key
    std::string character_id;       // Character UUID
    EntityId object_id;             // Object prototype (zone_id, local_id)
    std::optional<int> container_id; // Parent container item ID (if in a container)
    std::string equipped_location;  // Equipment slot name (empty = inventory)
    int condition = 100;            // Item condition (0-100)
    int charges = -1;               // Charges remaining (-1 = unlimited)
    std::vector<std::string> instance_flags;  // Per-instance flags
    std::string custom_name;        // Custom name override
    std::string custom_description; // Custom description override
};

/** Load all items for a character */
Result<std::vector<CharacterItemData>> load_character_items(
    pqxx::work& txn, const std::string& character_id);

/** Save a character's items (replaces all existing items) */
Result<void> save_character_items(
    pqxx::work& txn,
    const std::string& character_id,
    const std::vector<CharacterItemData>& items);

// =============================================================================
// Command Queries
// =============================================================================

/** Command data from the Command table */
struct CommandData {
    std::string name;                     // Primary command name (PK)
    std::vector<std::string> aliases;     // Alternative names
    std::string category;                 // CommandCategory enum value
    std::string description;              // Brief help text
    std::string usage;                    // Syntax example
    bool immortal_only = false;           // Requires immortal level
    std::vector<std::string> permissions; // Required permission flags
    std::optional<int> ability_id;        // Linked ability (if any)
};

/** Load all commands from database */
Result<std::vector<CommandData>> load_all_commands(pqxx::work& txn);

/** Load a single command by name */
Result<std::optional<CommandData>> load_command_by_name(
    pqxx::work& txn, const std::string& name);

} // namespace WorldQueries
