/***************************************************************************
 *   File: s../core/object.hpp                          Part of FieryMUD *
 *  Usage: Object hierarchy with modern APIs                               *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.       *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 ***************************************************************************/

#pragma once

#include "entity.hpp"
#include "../core/result.hpp"

#include <magic_enum/magic_enum.hpp>
#include <optional>
#include <unordered_map>
#include <unordered_set>

/** Object types for type-safe object handling */
enum class ObjectType {
    Undefined = 0,
    Light = 1,          // Light sources (torches, lanterns)
    Scroll = 2,         // Magic scrolls
    Wand = 3,           // Magic wands
    Staff = 4,          // Magic staves
    Weapon = 5,         // Weapons (swords, axes, etc.)
    Fireweapon = 6,     // Missile weapons (bows, crossbows)
    Missile = 7,        // Ammunition (arrows, bolts)
    Treasure = 8,       // Treasure items
    Armor = 9,          // Armor and clothing
    Potion = 10,        // Magic potions
    Worn = 11,          // Wearable non-armor items
    Other = 12,         // Miscellaneous items
    Trash = 13,         // Junk items
    Trap = 14,          // Trap objects
    Container = 15,     // Storage containers
    Note = 16,          // Readable notes
    Liquid_Container = 17, // Drink containers
    Key = 18,           // Keys for doors/containers
    Food = 19,          // Consumable food
    Money = 20,         // Currency
    Pen = 21,           // Writing implements
    Boat = 22,          // Transportation
    Fountain = 23,      // Water sources
    Vehicle = 24,       // Rideable vehicles
    Board = 25,         // Message boards
    Corpse = 26,        // Character remains
    Portal = 27,        // Magical portals
    Spellbook = 28,     // Spell books
    Kit = 29,           // Tool kits
    Wings = 30,         // Flight items
    Perfume = 31,       // Scent items
    Disguise = 32,      // Appearance modifiers
    Poison = 33         // Poison items
};

/** Equipment slots where objects can be worn/wielded */
enum class EquipSlot {
    None = -1,          // Not wearable
    Light = 0,          // Light source slot
    Finger_R = 1,       // Right finger
    Finger_L = 2,       // Left finger  
    Neck1 = 3,          // First neck slot
    Neck2 = 4,          // Second neck slot
    Body = 5,           // Main body armor
    Head = 6,           // Helmet/hat
    Legs = 7,           // Leg armor
    Feet = 8,           // Boots/shoes
    Hands = 9,          // Gloves
    Arms = 10,          // Arm protection
    Shield = 11,        // Shield
    About = 12,         // Cloak/cape
    Waist = 13,         // Belt
    Wrist_R = 14,       // Right wrist
    Wrist_L = 15,       // Left wrist
    Wield = 16,         // Main weapon
    Hold = 17,          // Held item
    Float = 18,         // Floating item
    Wield2 = 19,        // Second weapon
    Eye = 20,           // Eye wear
    Ear = 21,           // Earrings
    Badge = 22,         // Badge/pin
    Focus = 23,         // Spell focus
    Throat = 24,        // Throat protection
    Face = 25,          // Face covering
    Wings = 26,         // Wings slot
    Disguise = 27       // Disguise slot
};

/** Damage profile for weapons */
struct DamageProfile {
    int base_damage = 0;        // Base damage amount
    int dice_count = 0;         // Number of damage dice
    int dice_sides = 0;         // Sides per damage die
    int damage_bonus = 0;       // Flat damage bonus
    
    /** Calculate average damage */
    double average_damage() const {
        return base_damage + damage_bonus + (dice_count * (dice_sides + 1)) / 2.0;
    }
    
    /** Format as dice notation (e.g., "1d8+2") */
    std::string to_dice_string() const;
};

/** Extra description for detailed object examination */
struct ExtraDescription {
    std::vector<std::string> keywords;  // Keywords that trigger this description
    std::string description;            // The detailed description text
    
    /** Check if any keyword matches */
    bool matches_keyword(std::string_view keyword) const {
        for (const auto& kw : keywords) {
            if (kw == keyword) return true;
        }
        return false;
    }
};

/** Container properties */
struct ContainerInfo {
    int capacity = 0;           // Maximum items that can be stored
    int weight_capacity = 0;    // Maximum weight that can be stored
    bool closeable = false;     // Can be opened/closed
    bool closed = false;        // Current closed state
    bool lockable = false;      // Can be locked
    bool locked = false;        // Current locked state
    EntityId key_id = INVALID_ENTITY_ID;  // Key required to unlock
};

/** Light source properties */
struct LightInfo {
    int duration = 0;           // Hours of light remaining (-1 = infinite)
    int brightness = 1;         // Light intensity
    bool lit = false;           // Currently providing light
};

/** Object flags for special properties */
enum class ObjectFlag {
    Glow = 0,           // Glows with magical light
    Hum = 1,            // Hums with magical energy  
    NoRent = 2,         // Cannot be rented/saved
    NoDonate = 3,       // Cannot be donated
    NoInvis = 4,        // Cannot be made invisible
    Invisible = 5,      // Currently invisible
    Magic = 6,          // Has magical properties
    NoDrop = 7,         // Cannot be dropped
    Bless = 8,          // Blessed by gods
    AntiGood = 9,       // Harmful to good alignments
    AntiEvil = 10,      // Harmful to evil alignments
    AntiNeutral = 11,   // Harmful to neutral alignments
    AntiMage = 12,      // Cannot be used by mages
    AntiCleric = 13,    // Cannot be used by clerics
    AntiThief = 14,     // Cannot be used by thieves
    AntiWarrior = 15,   // Cannot be used by warriors
    NoSell = 16,        // Cannot be sold to shops
    AntiPaladin = 17,   // Cannot be used by paladins
    AntiRanger = 18,    // Cannot be used by rangers
    AntiBarbarian = 19, // Cannot be used by barbarians
    AntiSorcerer = 20,  // Cannot be used by sorcerers
    AntiRogue = 21,     // Cannot be used by rogues
    Unique = 22,        // Only one can exist
    Cursed = 23,        // Cursed item
    Identified = 24,    // Properties have been identified
    TwoHanded = 25,     // Requires both hands to wield
    Thrown = 26,        // Can be thrown as weapon
    Poison = 27,        // Coated with poison
    Enhanced = 28       // Magically enhanced
};

/** Modern Object class inheriting from Entity */
class Object : public Entity {
public:
    /** Create object with basic properties */
    static Result<std::unique_ptr<Object>> create(EntityId id, std::string_view name, ObjectType type);
    
    /** Load object from JSON */
    static Result<std::unique_ptr<Object>> from_json(const nlohmann::json& json);
    
    /** Virtual destructor */
    virtual ~Object() = default;
    
    /** Get object type */
    ObjectType type() const { return type_; }
    
    /** Get weight in pounds */
    int weight() const { return weight_; }
    
    /** Get base value in copper coins */
    int value() const { return value_; }
    
    /** Get object level requirement */
    int level() const { return level_; }
    
    /** Get wear slot if wearable */
    EquipSlot equip_slot() const { return equip_slot_; }
    
    /** Check if object can be worn/wielded */
    bool is_wearable() const { return equip_slot_ != EquipSlot::None; }
    
    /** Check if object is a weapon */
    bool is_weapon() const { 
        return type_ == ObjectType::Weapon || type_ == ObjectType::Fireweapon; 
    }
    
    /** Check if object is armor */
    bool is_armor() const { return type_ == ObjectType::Armor; }
    
    /** Check if object is a container */
    bool is_container() const { 
        return type_ == ObjectType::Container || type_ == ObjectType::Liquid_Container; 
    }
    
    /** Check if object is a light source */
    bool is_light_source() const { return type_ == ObjectType::Light; }
    
    /** Check if object has a specific flag */
    bool has_flag(ObjectFlag flag) const;
    
    /** Set object flag */
    void set_flag(ObjectFlag flag, bool value = true);
    
    /** Remove object flag */
    void remove_flag(ObjectFlag flag) { set_flag(flag, false); }
    
    /** Get damage profile (for weapons) */
    const DamageProfile& damage_profile() const { return damage_profile_; }
    
    /** Set damage profile */
    void set_damage_profile(const DamageProfile& profile) { damage_profile_ = profile; }
    
    /** Get container info (for containers) */
    const ContainerInfo& container_info() const { return container_info_; }
    
    /** Set container properties */
    void set_container_info(const ContainerInfo& info) { container_info_ = info; }
    
    /** Get light info (for light sources) */
    const LightInfo& light_info() const { return light_info_; }
    
    /** Set light properties */
    void set_light_info(const LightInfo& info) { light_info_ = info; }
    
    /** Get armor class bonus (for armor) */
    int armor_class() const { return armor_class_; }
    
    /** Set armor class */
    void set_armor_class(int ac) { armor_class_ = ac; }
    
    /** Get condition/durability (0-100) */
    int condition() const { return condition_; }
    
    /** Set condition */
    void set_condition(int cond) { 
        condition_ = std::max(0, std::min(100, cond)); 
    }
    
    /** Check if object is broken/destroyed */
    bool is_broken() const { return condition_ <= 0; }
    
    /** Get timer value (for timed objects) */
    int timer() const { return timer_; }
    
    /** Set timer value */
    void set_timer(int t) { timer_ = std::max(0, t); }
    
    /** Check if object has expired */
    bool is_expired() const { return timer_ == 0 && has_timer_; }
    
    /** Set properties */
    void set_weight(int w) { weight_ = std::max(0, w); }
    void set_value(int v) { value_ = std::max(0, v); }
    void set_level(int l) { level_ = std::max(0, l); }
    void set_equip_slot(EquipSlot slot) { equip_slot_ = slot; }
    void set_type(ObjectType new_type) { type_ = new_type; }
    
    /** JSON serialization */
    nlohmann::json to_json() const override;
    
    /** Get entity type name */
    std::string_view type_name() const override { return "Object"; }
    
    /** Validation */
    Result<void> validate() const override;
    
    /** Get formatted object name with condition */
    std::string display_name_with_condition(bool with_article = false) const;
    
    /** Get quality description based on condition */
    std::string_view quality_description() const;
    
    /** Extra description management */
    void add_extra_description(const ExtraDescription& extra_desc);
    std::optional<std::string_view> get_extra_description(std::string_view keyword) const;
    const std::vector<ExtraDescription>& get_all_extra_descriptions() const { return extra_descriptions_; }
    
    /** Get comprehensive stat information for debugging/admin commands */
    virtual std::string get_stat_info() const;
    
protected:
    /** Constructor for derived classes */
    Object(EntityId id, std::string_view name, ObjectType type);
    
private:
    ObjectType type_ = ObjectType::Undefined;
    int weight_ = 0;
    int value_ = 0;
    int level_ = 1;             // Level requirement for using object
    int condition_ = 100;       // 0-100 durability
    int timer_ = -1;            // Timer for temporary objects
    bool has_timer_ = false;    // Whether timer is active
    EquipSlot equip_slot_ = EquipSlot::None;
    int armor_class_ = 0;       // AC bonus for armor
    
    std::unordered_set<ObjectFlag> flags_;
    DamageProfile damage_profile_;
    ContainerInfo container_info_;
    LightInfo light_info_;
    std::vector<ExtraDescription> extra_descriptions_;
};

/** Specialized object types */

class Weapon : public Object {
public:
    static Result<std::unique_ptr<Weapon>> create(EntityId id, std::string_view name,
                                                ObjectType weapon_type = ObjectType::Weapon);
    
    std::string_view type_name() const override { return "Weapon"; }
    
    /** Get comprehensive stat information including weapon details */
    std::string get_stat_info() const override;
    
    /** Get weapon type (melee vs ranged) */
    bool is_ranged() const { return type() == ObjectType::Fireweapon; }
    
    /** Get weapon reach in feet */
    int reach() const { return reach_; }
    void set_reach(int r) { reach_ = std::max(0, r); }
    
    /** Get weapon speed (lower is faster) */
    int speed() const { return speed_; }
    void set_speed(int s) { speed_ = std::max(1, s); }
    
protected:
    Weapon(EntityId id, std::string_view name, ObjectType type);
    
private:
    int reach_ = 5;     // Weapon reach in feet
    int speed_ = 5;     // Attack speed (1-10, lower is faster)
};

class Armor : public Object {
public:
    static Result<std::unique_ptr<Armor>> create(EntityId id, std::string_view name, 
                                               EquipSlot slot = EquipSlot::Body);
    
    std::string_view type_name() const override { return "Armor"; }
    
    /** Get comprehensive stat information including armor details */
    std::string get_stat_info() const override;
    
    /** Get armor material type */
    std::string_view material() const { return material_; }
    void set_material(std::string_view mat) { material_ = mat; }
    
protected:
    Armor(EntityId id, std::string_view name, EquipSlot slot);
    
private:
    std::string material_ = "leather";
};

class Container : public Object {
public:
    static Result<std::unique_ptr<Container>> create(EntityId id, std::string_view name,
                                                   int capacity = 10);
    
    std::string_view type_name() const override { return "Container"; }
    
    /** Get comprehensive stat information including container details */
    std::string get_stat_info() const override;
    
    /** Container-specific operations */
    bool can_store_item(const Object& item) const;
    int current_capacity() const { return current_items_; }
    int current_weight() const { return current_weight_; }
    
    /** Container inventory management */
    Result<void> add_item(std::shared_ptr<Object> item);
    std::shared_ptr<Object> remove_item(EntityId item_id);
    bool remove_item(const std::shared_ptr<Object>& item);
    std::shared_ptr<Object> find_item(EntityId item_id) const;
    std::vector<std::shared_ptr<Object>> find_items_by_keyword(std::string_view keyword) const;
    std::span<const std::shared_ptr<Object>> get_contents() const;
    size_t contents_count() const { return contents_.size(); }
    bool is_empty() const { return contents_.empty(); }
    
    /** Update capacity tracking - deprecated, use add_item/remove_item */
    void add_item_tracking(int weight = 1) { 
        current_items_++;
        current_weight_ += weight;
    }
    
    void remove_item_tracking(int weight = 1) {
        current_items_ = std::max(0, current_items_ - 1);
        current_weight_ = std::max(0, current_weight_ - weight);
    }
    
protected:
    Container(EntityId id, std::string_view name, int capacity);
    
private:
    std::vector<std::shared_ptr<Object>> contents_;
    int current_items_ = 0;
    int current_weight_ = 0;
};

/** Object utility functions */
namespace ObjectUtils {
    /** Get object type name as string */
    std::string_view get_type_name(ObjectType type);
    
    /** Parse object type from string */
    std::optional<ObjectType> parse_object_type(std::string_view type_name);
    
    /** Get equip slot name as string */
    std::string_view get_slot_name(EquipSlot slot);
    
    /** Parse equip slot from string */
    std::optional<EquipSlot> parse_equip_slot(std::string_view slot_name);
    
    /** Check if object type can be equipped in slot */
    bool can_equip_in_slot(ObjectType type, EquipSlot slot);
    
    /** Get default equip slot for object type */
    EquipSlot get_default_slot(ObjectType type);
    
    /** Calculate object value based on properties */
    int calculate_base_value(ObjectType type, int level = 1);
    
    /** Get condition color code for display */
    std::string_view get_condition_color(int condition);
    
    /** Format damage profile as string */
    std::string format_damage_profile(const DamageProfile& damage);
}

/** JSON conversion support */
void to_json(nlohmann::json& json, const Object& object);
void from_json(const nlohmann::json& json, std::unique_ptr<Object>& object);

/** Formatting support for ObjectType and EquipSlot */
template<>
struct fmt::formatter<ObjectType> {
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    
    template<typename FormatContext>
    auto format(ObjectType type, FormatContext& ctx) const {
        return fmt::format_to(ctx.out(), "{}", ObjectUtils::get_type_name(type));
    }
};

template<>
struct fmt::formatter<EquipSlot> {
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    
    template<typename FormatContext>
    auto format(EquipSlot slot, FormatContext& ctx) const {
        return fmt::format_to(ctx.out(), "{}", ObjectUtils::get_slot_name(slot));
    }
};