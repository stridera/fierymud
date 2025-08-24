/***************************************************************************
 *   File: s../core/actor.hpp                           Part of FieryMUD *
 *  Usage: Actor base class with stats, inventory, and equipment           *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.       *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 ***************************************************************************/

#pragma once

#include "entity.hpp"
#include "object.hpp"
#include "../core/result.hpp"

#include <memory>
#include <vector>
#include <unordered_map>
#include <optional>
#include <span>

// Forward declarations
class Room;

/** Basic stats for all actors (players and NPCs) */
struct Stats {
    // Primary attributes
    int strength = 13;      // Physical power
    int dexterity = 13;     // Agility and reflexes
    int intelligence = 13;  // Mental acuity and learning
    int wisdom = 13;        // Awareness and judgment
    int constitution = 13;  // Health and endurance
    int charisma = 13;      // Force of personality
    
    // Derived stats
    int hit_points = 20;    // Current health
    int max_hit_points = 20; // Maximum health
    int mana = 100;         // Current magical energy
    int max_mana = 100;     // Maximum magical energy
    int movement = 100;     // Current movement points
    int max_movement = 100; // Maximum movement points
    
    // Combat stats
    int armor_class = 10;   // Defensive rating (lower is better)
    int hit_roll = 0;       // Attack bonus
    int damage_roll = 0;    // Damage bonus
    
    // Experience and progression
    int level = 1;          // Character level
    long experience = 0;    // Experience points
    long gold = 0;          // Currency in copper coins
    
    // Alignment and morality
    int alignment = 0;      // Good/Evil alignment (-1000 to 1000)
    
    /** Validate stats are within reasonable ranges */
    Result<void> validate() const;
    
    /** Calculate modifier for an attribute (D&D style: -5 to +5) */
    static int attribute_modifier(int attribute);
    
    /** Calculate next level experience requirement */
    long experience_to_next_level() const;
    
    /** Check if actor can level up */
    bool can_level_up() const;
    
    /** JSON serialization */
    nlohmann::json to_json() const;
    static Result<Stats> from_json(const nlohmann::json& json);
};

/** Inventory management for actors */
class Inventory {
public:
    /** Add item to inventory */
    Result<void> add_item(std::shared_ptr<Object> item);
    
    /** Remove item by EntityId */
    std::shared_ptr<Object> remove_item(EntityId item_id);
    
    /** Remove specific item instance */
    bool remove_item(const std::shared_ptr<Object>& item);
    
    /** Find item by EntityId */
    std::shared_ptr<Object> find_item(EntityId item_id) const;
    
    /** Find items matching keyword */
    std::vector<std::shared_ptr<Object>> find_items_by_keyword(std::string_view keyword) const;
    
    /** Get all items */
    std::span<const std::shared_ptr<Object>> get_all_items() const;
    
    /** Get total weight carried */
    int total_weight() const;
    
    /** Get total value of inventory */
    int total_value() const;
    
    /** Get number of items */
    size_t item_count() const { return items_.size(); }
    
    /** Check if inventory is empty */
    bool empty() const { return items_.empty(); }
    
    /** Clear all items (returns removed items) */
    std::vector<std::shared_ptr<Object>> clear();
    
    /** Check if can carry more weight */
    bool can_carry(int additional_weight, int max_carry_weight) const;
    
    /** JSON serialization */
    nlohmann::json to_json() const;
    static Result<Inventory> from_json(const nlohmann::json& json);
    
private:
    std::vector<std::shared_ptr<Object>> items_;
};

/** Equipment slots management */
class Equipment {
public:
    /** Equip item in appropriate slot */
    Result<void> equip_item(std::shared_ptr<Object> item);
    
    /** Unequip item from specific slot */
    std::shared_ptr<Object> unequip_item(EquipSlot slot);
    
    /** Unequip item by EntityId */
    std::shared_ptr<Object> unequip_item(EntityId item_id);
    
    /** Get item in specific slot */
    std::shared_ptr<Object> get_equipped(EquipSlot slot) const;
    
    /** Check if slot is occupied */
    bool is_equipped(EquipSlot slot) const;
    
    /** Get all equipped items */
    std::vector<std::shared_ptr<Object>> get_all_equipped() const;
    
    /** Get equipped items by type */
    std::vector<std::shared_ptr<Object>> get_equipped_by_type(ObjectType type) const;
    
    /** Calculate total armor class from equipped items */
    int calculate_total_armor_class() const;
    
    /** Get weapon in main hand */
    std::shared_ptr<Object> get_main_weapon() const;
    
    /** Get weapon in off hand */
    std::shared_ptr<Object> get_off_weapon() const;
    
    /** Check if wielding two-handed weapon */
    bool is_wielding_two_handed() const;
    
    /** Clear all equipment (returns removed items) */
    std::vector<std::shared_ptr<Object>> clear_all();
    
    /** JSON serialization */
    nlohmann::json to_json() const;
    static Result<Equipment> from_json(const nlohmann::json& json);
    
private:
    std::unordered_map<EquipSlot, std::shared_ptr<Object>> equipped_;
    
    /** Check if slot conflicts with currently equipped items */
    bool has_slot_conflict(EquipSlot slot, const Object& item) const;
};

/** Actor position in the world */
enum class Position {
    Dead = 0,           // Character is dead
    Mortally_Wounded,   // Dying but not dead
    Incapacitated,      // Cannot act
    Stunned,            // Briefly unable to act
    Sleeping,           // Asleep
    Resting,            // Resting to recover
    Sitting,            // Sitting down
    Fighting,           // In combat
    Standing            // Normal standing position
};

/** Actor state flags */
enum class ActorFlag {
    Blind = 0,          // Cannot see
    Invisible,          // Invisible to others
    Detect_Align,       // Can see alignments
    Detect_Invis,       // Can see invisible
    Detect_Magic,       // Can see magical auras
    Sense_Life,         // Can sense living beings
    Waterwalk,          // Can walk on water
    Sanctuary,          // Protected by sanctuary
    Group,              // Allows group formation
    Curse,              // Cursed
    Infravision,        // Can see in dark
    Poison,             // Poisoned
    Protect_Evil,       // Protected from evil
    Protect_Good,       // Protected from good
    Sleep,              // Magically asleep
    No_Track,           // Cannot be tracked
    Flying,             // Can fly
    Underwater_Breathing, // Can breathe underwater
    Sneak,              // Moving stealthily
    Hide,               // Hiding from view
    Charm,              // Charmed by another
    Follow,             // Following someone
    Wimpy,              // Flees when low on HP
    No_Summon,          // Cannot be summoned
    No_Teleport         // Cannot be teleported
};

/** Base Actor class for all living beings */
class Actor : public Entity {
public:
    /** Virtual destructor */
    virtual ~Actor() = default;
    
    /** Get actor stats */
    const Stats& stats() const { return stats_; }
    Stats& stats() { return stats_; }
    
    /** Get inventory */
    const Inventory& inventory() const { return inventory_; }
    Inventory& inventory() { return inventory_; }
    
    /** Get equipment */
    const Equipment& equipment() const { return equipment_; }
    Equipment& equipment() { return equipment_; }
    
    /** Get current position */
    Position position() const { return position_; }
    
    /** Set position */
    void set_position(Position pos) { position_ = pos; }
    
    /** Check if actor has a flag */
    bool has_flag(ActorFlag flag) const;
    
    /** Set actor flag */
    void set_flag(ActorFlag flag, bool value = true);
    
    /** Remove actor flag */
    void remove_flag(ActorFlag flag) { set_flag(flag, false); }
    
    /** Get current room (weak reference to avoid cycles) */
    std::shared_ptr<Room> current_room() const;
    
    /** Set current room */
    void set_current_room(std::weak_ptr<Room> room) { current_room_ = room; }
    
    /** Movement and positioning */
    virtual Result<void> move_to(std::shared_ptr<Room> new_room);
    
    /** Communication */
    virtual void send_message(std::string_view message) = 0;
    virtual void receive_message(std::string_view message) = 0;
    
    /** Combat state */
    bool is_fighting() const { return position_ == Position::Fighting; }
    bool is_alive() const { return position_ != Position::Dead; }
    bool can_act() const { 
        return is_alive() && position_ != Position::Incapacitated && 
               position_ != Position::Stunned && position_ != Position::Sleeping;
    }
    
    /** Visibility checks */
    bool is_visible_to(const Actor& observer) const;
    bool can_see(const Actor& target) const;
    bool can_see(const Object& object) const;
    
    /** Item management convenience methods */
    Result<void> give_item(std::shared_ptr<Object> item);
    std::shared_ptr<Object> take_item(EntityId item_id);
    Result<void> equip(EntityId item_id);
    std::shared_ptr<Object> unequip(EquipSlot slot);
    
    /** Get carrying capacity */
    int max_carry_weight() const;
    int current_carry_weight() const;
    bool is_overloaded() const;
    
    /** Experience and leveling */
    void gain_experience(long amount);
    virtual Result<void> level_up();
    
    /** JSON serialization */
    nlohmann::json to_json() const override;
    
    /** Get entity type name */
    std::string_view type_name() const override { return "Actor"; }
    
    /** Validation */
    Result<void> validate() const override;
    
protected:
    /** Constructor for derived classes */
    Actor(EntityId id, std::string_view name);
    
    /** Constructor for loading from JSON */
    Actor(EntityId id, std::string_view name, 
          const Stats& stats, const Inventory& inventory, const Equipment& equipment);
    
    /** Room change notification (override in derived classes) */
    virtual void on_room_change(std::shared_ptr<Room> old_room, std::shared_ptr<Room> new_room) {}
    
    /** Level up notification (override in derived classes) */
    virtual void on_level_up(int old_level, int new_level) {}
    
private:
    Stats stats_;
    Inventory inventory_;
    Equipment equipment_;
    Position position_ = Position::Standing;
    std::unordered_set<ActorFlag> flags_;
    std::weak_ptr<Room> current_room_;
};

/** Mobile (NPC) class */
class Mobile : public Actor {
public:
    /** Create mobile */
    static Result<std::unique_ptr<Mobile>> create(EntityId id, std::string_view name, int level = 1);
    
    /** Load from JSON */
    static Result<std::unique_ptr<Mobile>> from_json(const nlohmann::json& json);
    
    /** Get entity type name */
    std::string_view type_name() const override { return "Mobile"; }
    
    /** Communication (NPCs store messages for AI processing) */
    void send_message(std::string_view message) override;
    void receive_message(std::string_view message) override;
    
    /** AI behavior properties */
    bool is_aggressive() const { return aggressive_; }
    void set_aggressive(bool value) { aggressive_ = value; }
    
    int aggression_level() const { return aggression_level_; }
    void set_aggression_level(int level) { aggression_level_ = std::max(0, std::min(10, level)); }
    
    /** Memory for received messages (for AI) */
    const std::vector<std::string>& get_received_messages() const { return received_messages_; }
    void clear_received_messages() { received_messages_.clear(); }
    
protected:
    Mobile(EntityId id, std::string_view name, int level = 1);
    
private:
    bool aggressive_ = false;
    int aggression_level_ = 5;  // 0-10 scale
    std::vector<std::string> received_messages_;
    
    void initialize_for_level(int level);
};

/** Player class */
class Player : public Actor {
public:
    /** Create player */
    static Result<std::unique_ptr<Player>> create(EntityId id, std::string_view name);
    
    /** Load from JSON */
    static Result<std::unique_ptr<Player>> from_json(const nlohmann::json& json);
    
    /** Get entity type name */
    std::string_view type_name() const override { return "Player"; }
    
    /** Communication (Players queue output to session) */
    void send_message(std::string_view message) override;
    void receive_message(std::string_view message) override;
    
    /** Player-specific properties */
    std::string_view account() const { return account_; }
    void set_account(std::string_view account) { account_ = account; }
    
    bool is_online() const { return online_; }
    void set_online(bool value) { online_ = value; }
    
    /** Output queue for session handling */
    const std::vector<std::string>& get_output_queue() const { return output_queue_; }
    void clear_output_queue() { output_queue_.clear(); }
    
    /** Player privileges */
    bool is_god() const { return god_level_ > 0; }
    int god_level() const { return god_level_; }
    void set_god_level(int level) { god_level_ = std::max(0, level); }
    
    /** GMCP support methods */
    nlohmann::json get_vitals_gmcp() const;
    nlohmann::json get_status_gmcp() const;
    
    /** Player output interface for network layer */
    void set_output(std::shared_ptr<class PlayerOutput> output) { output_ = output; }
    std::shared_ptr<class PlayerOutput> get_output() const { return output_; }
    
    /** Convenience methods for network layer */  
    std::shared_ptr<Room> current_room_ptr() const { return current_room(); }
    int level() const { return stats().level; }
    std::string player_class() const { return "warrior"; } // TODO: implement proper class system
    std::string race() const { return "human"; } // TODO: implement proper race system
    
protected:
    Player(EntityId id, std::string_view name);
    
private:
    std::string account_;
    bool online_ = false;
    int god_level_ = 0;
    std::vector<std::string> output_queue_;
    std::shared_ptr<class PlayerOutput> output_;
};

/** Actor utility functions */
namespace ActorUtils {
    /** Calculate experience required for level */
    long experience_for_level(int level);
    
    /** Calculate hit points for level and constitution */
    int calculate_hit_points(int level, int constitution);
    
    /** Calculate mana for level and intelligence */
    int calculate_mana(int level, int intelligence);
    
    /** Calculate movement for level and constitution */
    int calculate_movement(int level, int constitution);
    
    /** Format actor stats for display */
    std::string format_stats(const Stats& stats);
    
    /** Get position name as string */
    std::string_view get_position_name(Position position);
    
    /** Parse position from string */
    std::optional<Position> parse_position(std::string_view position_name);
    
    /** Get flag name as string */
    std::string_view get_flag_name(ActorFlag flag);
    
    /** Parse flag from string */
    std::optional<ActorFlag> parse_flag(std::string_view flag_name);
}

/** JSON conversion support */
void to_json(nlohmann::json& json, const Stats& stats);
void from_json(const nlohmann::json& json, Stats& stats);
void to_json(nlohmann::json& json, const Actor& actor);
void from_json(const nlohmann::json& json, std::unique_ptr<Actor>& actor);

/** Formatting support */
template<>
struct fmt::formatter<Position> {
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    
    template<typename FormatContext>
    auto format(Position pos, FormatContext& ctx) const {
        return fmt::format_to(ctx.out(), "{}", ActorUtils::get_position_name(pos));
    }
};

template<>
struct fmt::formatter<ActorFlag> {
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    
    template<typename FormatContext>
    auto format(ActorFlag flag, FormatContext& ctx) const {
        return fmt::format_to(ctx.out(), "{}", ActorUtils::get_flag_name(flag));
    }
};