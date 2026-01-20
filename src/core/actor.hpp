#pragma once

#include "entity.hpp"
#include "money.hpp"
#include "object.hpp"
#include "active_effect.hpp"
#include "../core/result.hpp"
#include "../database/generated/db_enums.hpp"

#include <memory>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <optional>
#include <span>
#include <chrono>
#include <ctime>
#include <bitset>

#include "spell_system.hpp"

// Forward declarations
class Room;
class PlayerConnection;
class ComposerSystem;

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
    int stamina = 100;      // Current stamina
    int max_stamina = 100;  // Maximum stamina
    
    // Offensive Stats (ACC/EVA combat system)
    int accuracy = 0;           // Attack accuracy (replaces THAC0/hitroll)
    int attack_power = 0;       // Physical damage bonus (replaces damroll)
    int spell_power = 0;        // Magical damage bonus
    int penetration_flat = 0;   // Flat armor penetration
    int penetration_percent = 0; // Percent armor penetration (0-100)

    // New Defensive Stats
    int evasion = 0;            // Dodge chance (replaces AC partially)
    int armor_rating = 0;       // Damage reduction from armor (replaces AC partially)
    int damage_reduction_percent = 0; // Percent damage reduction (0-100, capped at 75%)
    int soak = 0;               // Flat damage reduction per hit
    int hardness = 0;           // Damage threshold (ignore hits below this)
    int ward_percent = 0;       // Percent magical damage reduction (0-100)

    // Elemental Resistances (0-100, can be negative for vulnerability)
    int resistance_fire = 0;
    int resistance_cold = 0;
    int resistance_lightning = 0;
    int resistance_acid = 0;
    int resistance_poison = 0;

    // Detection and stealth
    int perception = 0;     // Ability to detect hidden things
    int concealment = 0;    // Ability to hide from detection

    // Spell memorization
    int focus = 0;          // Improves spell memorization speed

    // Experience and progression
    int level = 1;          // Character level
    long experience = 0;    // Experience points
    long gold = 0;          // Currency in copper coins
    
    // Alignment and morality
    int alignment = 0;      // Good/Evil alignment (-1000 to 1000)

    // Condition system (hunger/thirst/drunkenness) - range 0-24
    // Note: 24 = satiated/drunk, 0 = starving/parched/sober
    int drunk = 0;          // Intoxication level (0=sober, >6=slurred, >10=too drunk)
    int hunger = 24;        // Hunger level (24=full, 0=starving)
    int thirst = 24;        // Thirst level (24=quenched, 0=parched)

    // Condition constants
    static constexpr int CONDITION_MIN = 0;
    static constexpr int CONDITION_MAX = 24;
    static constexpr int DRUNK_SLURRED_THRESHOLD = 6;   // Speech becomes slurred
    static constexpr int DRUNK_TOO_DRUNK_THRESHOLD = 10; // Can't drink more
    static constexpr int CONDITION_HUNGRY_THRESHOLD = 4; // Starting to feel hungry
    static constexpr int CONDITION_THIRSTY_THRESHOLD = 4; // Starting to feel thirsty

    /** Gain condition (drunk/hunger/thirst) - clamps to valid range */
    void gain_condition(int& condition, int amount) {
        condition = std::clamp(condition + amount, CONDITION_MIN, CONDITION_MAX);
    }

    /** Check if too drunk to drink more */
    bool is_too_drunk() const { return drunk > DRUNK_TOO_DRUNK_THRESHOLD; }

    /** Check if speech is slurred */
    bool is_slurring() const { return drunk > DRUNK_SLURRED_THRESHOLD; }

    /** Check if hungry (hunger below threshold) */
    bool is_hungry() const { return hunger < CONDITION_HUNGRY_THRESHOLD; }

    /** Check if thirsty (thirst below threshold) */
    bool is_thirsty() const { return thirst < CONDITION_THIRSTY_THRESHOLD; }

    /** Check if starving (hunger at 0) */
    bool is_starving() const { return hunger == CONDITION_MIN; }

    /** Check if parched (thirst at 0) */
    bool is_parched() const { return thirst == CONDITION_MIN; }

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

    /** Equip item to a specific slot (for loading from database) */
    Result<void> equip_to_slot(std::shared_ptr<Object> item, EquipSlot slot);

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
    
    /** Get all equipped items with their slots */
    std::vector<std::pair<EquipSlot, std::shared_ptr<Object>>> get_all_equipped_with_slots() const;
    
    /** Get equipped items by type */
    std::vector<std::shared_ptr<Object>> get_equipped_by_type(ObjectType type) const;
    
    /** Calculate total armor class from equipped items */
    int calculate_total_armor_class() const;
    
    /** Get weapon in main hand */
    std::shared_ptr<Object> get_main_weapon() const;
    
    /** Get weapon in off hand */
    std::shared_ptr<Object> get_off_weapon() const;
    
    /** Check if wielding two-handed weapon (or versatile weapon with 2H grip) */
    bool is_wielding_two_handed() const;

    /** Check if currently using two-handed grip on a versatile weapon */
    bool is_using_two_handed_grip() const { return using_two_handed_grip_; }

    /** Toggle grip on a versatile weapon (1H <-> 2H)
     * @return Error message if failed, empty string on success */
    std::string toggle_grip();

    /** Reset grip to one-handed (called when weapon is unequipped) */
    void reset_grip() { using_two_handed_grip_ = false; }

    /** Clear all equipment (returns removed items) */
    std::vector<std::shared_ptr<Object>> clear_all();
    
    /** JSON serialization */
    nlohmann::json to_json() const;
    static Result<Equipment> from_json(const nlohmann::json& json);
    
private:
    std::unordered_map<EquipSlot, std::shared_ptr<Object>> equipped_;
    bool using_two_handed_grip_ = false;  // True if using 2H grip on versatile weapon

    /** Check if slot conflicts with currently equipped items */
    bool has_slot_conflict(EquipSlot slot, const Object& item) const;

    /** Get descriptive error message for slot conflicts */
    std::string get_slot_conflict_message(EquipSlot slot, const Object& item) const;
};

/** Actor position in the world */
enum class Position {
    Dead = 0,           // Character is dead
    Ghost,              // Character is dead but in ghost form
    Mortally_Wounded,   // Dying but not dead
    Incapacitated,      // Cannot act
    Stunned,            // Briefly unable to act
    Sleeping,           // Asleep
    Resting,            // Resting to recover
    Sitting,            // Sitting down
    Prone,              // Lying down (knocked down or resting flat)
    Fighting,           // In combat
    Standing,           // Normal standing position
    Flying              // Flying/hovering in the air
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
    No_Teleport,        // Cannot be teleported
    // Spell-applied status flags
    Bless,              // Blessed (+hitroll, +saves)
    Armor,              // Magical armor (+AC)
    Shield,             // Shield spell (+AC)
    Stoneskin,          // Stoneskin protection
    Barkskin,           // Barkskin protection (thick, brown skin)
    Haste,              // Moving faster
    Slow,               // Moving slower
    Strength,           // Enhanced strength
    Weakness,           // Reduced strength
    Blur,               // Hard to hit
    Mirror_Image,       // Has illusory copies
    Protection_Evil,    // Protected from evil
    Protection_Good,    // Protected from good
    Fireshield,         // Fire shield active
    Coldshield,         // Cold shield active
    Aware,              // Cannot be surprised
    Berserk,            // In berserker rage
    Taunted,            // Must attack taunter
    Webbed,             // Stuck in webs
    Paralyzed,          // Cannot move
    Glowing,            // Emitting light
    Meditating,         // In meditative state
    // Additional detection and status flags
    Detect_Poison,      // Can see poisoned creatures
    On_Fire,            // Currently burning
    Immobilized,        // Cannot move (roots, webs, etc.)
    // Food/drink buffs
    Refreshed,          // Well-hydrated from drinking (+50% stamina regen)
    Nourished,          // Well-fed from eating (+50% HP regen)
    // Sentinel value for effects that don't set a flag
    None                // No flag (for cooldowns, debuffs without visual effects, etc.)
};

/** Active effect on an actor (spell effect, buff, debuff) */
struct ActiveEffect {
    int effect_id = 0;          // Database Effect ID (for persistence)
    std::string name;           // Effect name (e.g., "Armor", "Bless")
    std::string source;         // What applied this (spell name, item, etc.)
    ActorFlag flag;             // Associated flag (if any)
    double duration_hours;      // Remaining duration in MUD hours (-1 = permanent, supports fractional hours)
    int modifier_value;         // Stat modifier value (e.g., +10 AC)
    std::string modifier_stat;  // What stat is modified (e.g., "armor_class")
    std::chrono::steady_clock::time_point applied_at;  // When effect was applied

    bool is_permanent() const { return duration_hours < 0; }
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

    /** Set position (triggers on_position_change if position actually changes) */
    void set_position(Position pos) {
        if (position_ != pos) {
            Position old_pos = position_;
            position_ = pos;
            on_position_change(old_pos, pos);
        }
    }
    
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
    bool is_alive() const { return position_ != Position::Dead && position_ != Position::Ghost; }
    bool can_act() const {
        return is_alive() && position_ != Position::Incapacitated &&
               position_ != Position::Stunned && position_ != Position::Sleeping;
    }

    /** Fighting list management - tracks who this actor is fighting */
    void add_enemy(std::shared_ptr<Actor> enemy);
    void remove_enemy(std::shared_ptr<Actor> enemy);
    void clear_enemies();
    bool has_enemies() const;
    std::shared_ptr<Actor> get_fighting_target() const;  // First valid enemy
    std::vector<std::shared_ptr<Actor>> get_all_enemies() const;  // For warriors/debugging
    void set_primary_target(std::shared_ptr<Actor> enemy);  // Move to front of list

    /** Mounting state */
    bool is_mounted() const { return !mounted_on_.expired(); }
    bool has_rider() const { return !rider_.expired(); }
    std::shared_ptr<Actor> get_mount() const { return mounted_on_.lock(); }
    std::shared_ptr<Actor> get_rider() const { return rider_.lock(); }

    void set_rider(std::shared_ptr<Actor> rider) { rider_ = rider; }
    void set_mounted_on(std::shared_ptr<Actor> mount) { mounted_on_ = mount; }

    void dismount() {
        if (auto mount = mounted_on_.lock()) {
            mount->rider_.reset();
        }
        mounted_on_.reset();
    }

    /** Handle death - implemented differently by Player (becomes ghost) and Mobile (creates corpse, despawns)
     *  Returns the corpse for Mobile deaths, nullptr for Player deaths */
    virtual std::shared_ptr<Container> die() = 0;
    
    /** Visibility checks */
    bool is_visible_to(const Actor& observer) const;
    bool can_see(const Actor& target) const;
    bool can_see(const Object& object) const;

    /** Status checks (virtual for Player override) */
    virtual bool is_afk() const { return false; }
    virtual bool is_holylight() const { return false; }
    virtual bool is_show_ids() const { return false; }

    /** Get display name with detection indicators for an observer
     * Returns display_name() plus any applicable indicators like (invis), (evil), (good)
     * based on what the observer can detect via Detect_Invis, Detect_Align, etc.
     */
    std::string display_name_for_observer(const Actor& observer) const;

    /**
     * Override room_presence for position-based descriptions.
     * Returns descriptions like "Strider is standing here." or "A goblin is fighting YOU!"
     */
    std::string room_presence(std::shared_ptr<Actor> viewer = nullptr) const override;

    /** Item management convenience methods */
    Result<void> give_item(std::shared_ptr<Object> item);
    std::shared_ptr<Object> take_item(EntityId item_id);
    Result<void> equip(EntityId item_id);
    std::shared_ptr<Object> unequip(EquipSlot slot);
    
    /** Get carrying capacity */
    int max_carry_weight() const;
    int current_carry_weight() const;
    bool is_overloaded() const;

    // =========================================================================
    // Unified Wealth Interface (in copper)
    // =========================================================================

    /** Get wealth in copper (Players use wallet, Mobiles use money) */
    virtual long wealth() const = 0;

    /** Give wealth in copper */
    virtual void give_wealth(long copper_amount) = 0;

    /** Take wealth in copper - returns true if sufficient funds */
    virtual bool take_wealth(long copper_amount) = 0;

    /** Check if actor can afford amount in copper */
    virtual bool can_afford(long copper_amount) const = 0;

    /** Experience and leveling */
    void gain_experience(long amount);
    virtual Result<void> level_up();
    
    /** JSON serialization */
    nlohmann::json to_json() const override;
    
    /** Get entity type name */
    std::string_view type_name() const override { return "Actor"; }

    /** Get actor's look description (virtual so subclasses can override) */
    virtual std::string_view description() const { return ""; }
    virtual void set_description(std::string_view desc) { (void)desc; }  // Base does nothing
    
    /** Validation */
    Result<void> validate() const override;
    
    /** Get comprehensive stat information for debugging/admin commands */
    std::string get_stat_info() const;
    
    /** Spell casting support */
    bool can_cast_spell(std::string_view spell_name) const;
    bool has_spell_slots(int circle) const;
    bool consume_spell_slot(int circle);  // Consumes slot and adds to restoration queue
    std::pair<int, int> get_spell_slot_info(int circle) const;
    const SpellSlots& spell_slots() const;
    SpellSlots& spell_slots();

    /** Active effect management (legacy spell effects) */
    void add_effect(const ActiveEffect& effect);
    void remove_effect(const std::string& effect_name);
    bool has_effect(const std::string& effect_name) const;
    const ActiveEffect* get_effect(const std::string& effect_name) const;
    const std::vector<ActiveEffect>& active_effects() const { return active_effects_; }
    void tick_effects();  // Called each game round to decrement durations
    void clear_effects();

    // =========================================================================
    // Data-Driven DoT Effect System
    // =========================================================================

    /** Add a DoT effect to this actor */
    void add_dot_effect(const fiery::DotEffect& effect);

    /** Remove all DoT effects matching a cure category */
    void remove_dot_effects_by_category(const std::string& cure_category);

    /** Get all active DoT effects */
    const std::vector<fiery::DotEffect>& dot_effects() const { return dot_effects_; }
    std::vector<fiery::DotEffect>& dot_effects() { return dot_effects_; }

    /** Check if actor has any DoT effects of a given category */
    bool has_dot_effect(const std::string& cure_category) const;

    /** Check if any DoT effect blocks regeneration */
    bool is_regen_blocked_by_dot() const;

    /** Get total regen reduction from all DoT effects (0-100) */
    int get_dot_regen_reduction() const;

    /** Attempt to cure DoT effects of a given category
     * @param cure_category Category to cure ("poison", "fire", "disease", "all")
     * @param cure_power Power of the cure (must be >= effect potency for full cure)
     * @param cure_all If true, attempt to cure all matching effects; if false, only strongest
     * @param partial_on_fail If true, partial cure when underpowered; if false, total failure
     * @return Result of the cure attempt with message and counts
     */
    fiery::CureAttemptResult attempt_cure(const std::string& cure_category,
                                          int cure_power,
                                          bool cure_all = false,
                                          bool partial_on_fail = true);

    /** Process all DoT effects for one tick - returns damage dealt by type */
    struct DotTickResult {
        int total_damage = 0;
        std::vector<std::pair<std::string, int>> damage_by_type;  // (damage_type, amount)
        std::vector<std::string> expired_effects;
        bool died = false;
    };
    DotTickResult process_dot_effects();

    /** Update actor flags based on active DoT effects (e.g., set Poison flag if poisoned) */
    void update_dot_flags();

    // =========================================================================
    // Data-Driven HoT (Heal Over Time) Effect System
    // =========================================================================

    /** Add a HoT effect to this actor */
    void add_hot_effect(const fiery::HotEffect& effect);

    /** Remove all HoT effects matching a dispel category */
    void remove_hot_effects_by_category(const std::string& dispel_category);

    /** Get all active HoT effects */
    const std::vector<fiery::HotEffect>& hot_effects() const { return hot_effects_; }
    std::vector<fiery::HotEffect>& hot_effects() { return hot_effects_; }

    /** Check if actor has any HoT effects of a given category */
    bool has_hot_effect(const std::string& hot_category) const;

    /** Check if any HoT effect boosts regeneration */
    bool is_regen_boosted_by_hot() const;

    /** Get total regen boost from all HoT effects (0-100+) */
    int get_hot_regen_boost() const;

    /** Process all HoT effects for one tick - returns healing done */
    fiery::HotTickResult process_hot_effects();

    /** Result of tick processing - contains changes and events that occurred */
    struct TickResult {
        int hp_gained = 0;
        int stamina_gained = 0;
        int hot_healing = 0;          // Healing from HoT effects
        int poison_damage = 0;
        int dying_damage = 0;
        std::vector<std::string> expired_effects;
        bool died = false;
    };

    /** Fast regeneration tick (called every 4 real seconds like legacy)
     * Handles: HP/stamina regeneration, DoT/HoT effects, dying damage
     * Returns info about what happened for messaging */
    virtual TickResult perform_regen_tick();

    /** Hourly tick processing (called each MUD hour = 75 real seconds)
     * Handles: Spell effect duration decrements, condition depletion (drunk)
     * Returns info about what happened for messaging */
    virtual TickResult perform_hour_tick();

    /** Legacy compatibility - calls perform_regen_tick() */
    virtual TickResult perform_tick() { return perform_regen_tick(); }

    /** Calculate HP regeneration per tick (adjusted for 4-second ticks) */
    virtual int calculate_hit_gain() const;

    /** Calculate stamina regeneration per tick (adjusted for 4-second ticks) */
    virtual int calculate_stamina_gain() const;

    /** Get position-based regeneration multiplier (1.0 = normal) */
    double get_regen_multiplier() const;

    /** Interrupt concentration-based activities like meditation */
    bool interrupt_concentration();

    /** Casting blackout system - prevents spam-casting */
    bool is_casting_blocked() const;
    std::chrono::milliseconds get_blackout_remaining() const;
    void set_casting_blackout(std::chrono::milliseconds duration);
    void clear_casting_blackout();

    /** Spell queue system - allows queuing next spell during blackout */
    struct QueuedSpell {
        std::string spell_name;
        std::string target_name;
        std::chrono::steady_clock::time_point queued_at;
    };
    bool has_queued_spell() const { return queued_spell_.has_value(); }
    const std::optional<QueuedSpell>& queued_spell() const { return queued_spell_; }
    void queue_spell(std::string_view spell_name, std::string_view target_name);
    void clear_queued_spell() { queued_spell_.reset(); }
    std::optional<QueuedSpell> pop_queued_spell();

    /** Casting state - tracks spell being cast with casting time */
    struct CastingState {
        int ability_id = 0;             // Ability being cast
        std::string ability_name;       // Display name for messages
        int ticks_remaining = 0;        // Casting time remaining (in pulses, each pulse ~0.5s)
        int total_ticks = 0;            // Total casting time (for progress display)
        std::weak_ptr<Actor> target;    // Target actor (if any)
        std::string target_name;        // Target name for retargeting
        int circle = 0;                 // Spell circle (for slot consumption)
        bool quickcast_applied = false; // Whether quickcast reduction was applied
    };
    bool is_casting() const { return casting_state_.has_value(); }
    const std::optional<CastingState>& casting_state() const { return casting_state_; }
    void start_casting(const CastingState& state);
    void stop_casting(bool interrupted = false);
    bool advance_casting();  // Returns true when casting completes
    int get_casting_progress_percent() const;

protected:
    /** Constructor for derived classes */
    Actor(EntityId id, std::string_view name);
    
    /** Constructor for loading from JSON */
    Actor(EntityId id, std::string_view name, 
          const Stats& stats, const Inventory& inventory, const Equipment& equipment);
    
    /** Room change notification (override in derived classes) */
    virtual void on_room_change([[maybe_unused]] std::shared_ptr<Room> old_room,
                               [[maybe_unused]] std::shared_ptr<Room> new_room) {}

    /** Position change notification (override in derived classes for GMCP updates) */
    virtual void on_position_change([[maybe_unused]] Position old_pos,
                                    [[maybe_unused]] Position new_pos) {}

    /** Level up notification (override in derived classes) */
    virtual void on_level_up([[maybe_unused]] int old_level, [[maybe_unused]] int new_level) {}

public:
    /** Gender and size - common to all actors (players and mobiles) */
    std::string_view gender() const { return gender_; }
    void set_gender(std::string_view g) { gender_ = g; }

    std::string_view size() const { return size_; }
    void set_size(std::string_view s) { size_ = s; }

    virtual std::string_view race() const { return race_; }
    void set_race(std::string_view r) { race_ = r; }

private:
    Stats stats_;
    Inventory inventory_;
    Equipment equipment_;
    Position position_ = Position::Standing;
    std::unordered_set<ActorFlag> flags_;
    std::weak_ptr<Room> current_room_;
    std::unique_ptr<SpellSlots> spell_slots_;
    std::string gender_ = "Neuter";
    std::string size_ = "Medium";
    std::string race_ = "Human";
    std::vector<ActiveEffect> active_effects_;
    std::vector<fiery::DotEffect> dot_effects_;  // Data-driven DoT effects
    std::vector<fiery::HotEffect> hot_effects_;  // Data-driven HoT effects

    // Casting blackout system
    std::chrono::steady_clock::time_point casting_blackout_end_{};
    std::optional<QueuedSpell> queued_spell_{};

    // Casting state - for spells with casting time
    std::optional<CastingState> casting_state_{};

    // Mounting state
    std::weak_ptr<Actor> mounted_on_;  // What we are riding
    std::weak_ptr<Actor> rider_;       // Who is riding us

    // Pet/follower state (for charmed mobs, pets, mounts)
    std::weak_ptr<Actor> master_;                    // Who we are following/serving
    std::vector<std::weak_ptr<Actor>> followers_;    // Who is following us

    // Combat state - list of actors we are fighting
    std::vector<std::weak_ptr<Actor>> fighting_list_;  // Enemies we are engaged with

public:
    // Master/follower system (for pets, charmed mobs, etc.)
    std::shared_ptr<Actor> get_master() const { return master_.lock(); }
    bool has_master() const { return !master_.expired(); }
    void set_master(std::shared_ptr<Actor> master) { master_ = master; }
    void clear_master() { master_.reset(); }

    const std::vector<std::weak_ptr<Actor>>& get_followers() const { return followers_; }
    bool has_followers() const { return !followers_.empty(); }

    void add_follower(std::shared_ptr<Actor> follower) {
        // Clean up expired weak pointers first
        followers_.erase(
            std::remove_if(followers_.begin(), followers_.end(),
                [](const std::weak_ptr<Actor>& wp) { return wp.expired(); }),
            followers_.end());
        followers_.push_back(follower);
    }

    void remove_follower(std::shared_ptr<Actor> follower) {
        followers_.erase(
            std::remove_if(followers_.begin(), followers_.end(),
                [&follower](const std::weak_ptr<Actor>& wp) {
                    auto sp = wp.lock();
                    return !sp || sp == follower;
                }),
            followers_.end());
    }
};

/** Mobile (NPC) trait/behavior/profession enums - uses database-defined enums directly */
using MobTrait = db::MobTrait;
using MobBehavior = db::MobBehavior;
using MobProfession = db::MobProfession;

/** Mobile (NPC) class */
class Mobile : public Actor {
public:
    /** Capacity for bitsets - must accommodate highest enum value + 1 */
    static constexpr size_t MOB_TRAIT_CAPACITY = 16;
    static constexpr size_t MOB_BEHAVIOR_CAPACITY = 32;
    static constexpr size_t MOB_PROFESSION_CAPACITY = 16;

    /** Create mobile */
    static Result<std::unique_ptr<Mobile>> create(EntityId id, std::string_view name, int level = 1);
    
    /** Load from JSON */
    static Result<std::unique_ptr<Mobile>> from_json(const nlohmann::json& json);
    
    /** Get entity type name */
    std::string_view type_name() const override { return "Mobile"; }
    
    /** Communication (NPCs store messages for AI processing) */
    void send_message(std::string_view message) override;
    void receive_message(std::string_view message) override;

    /** Death handling - creates corpse, removes from room, despawns
     *  Returns the created corpse */
    std::shared_ptr<Container> die() override;
    
    /** AI behavior properties - aggro_condition is a Lua expression from database
     *  Examples: "true" (attacks all), "target.alignment <= -350" (attacks evil) */
    const std::optional<std::string>& aggro_condition() const { return aggro_condition_; }
    void set_aggro_condition(std::optional<std::string> cond) { aggro_condition_ = std::move(cond); }

    /** Check if this mob has any aggression condition set */
    bool is_aggressive() const { return aggro_condition_.has_value() && !aggro_condition_->empty(); }
    
    /** Shopkeeper properties */
    bool is_shopkeeper() const { return is_shopkeeper_; }
    void set_shopkeeper(bool value) { is_shopkeeper_ = value; }

    /** Banker properties */
    bool is_banker() const { return is_banker_; }
    void set_banker(bool value) { is_banker_ = value; }

    /** Receptionist properties (inn/lodging) */
    bool is_receptionist() const { return is_receptionist_; }
    void set_receptionist(bool value) { is_receptionist_ = value; }

    /** Postmaster properties (mail) */
    bool is_postmaster() const { return is_postmaster_; }
    void set_postmaster(bool value) { is_postmaster_ = value; }

    /** Prototype ID (for spawned mobs, references the prototype they were created from) */
    EntityId prototype_id() const { return prototype_id_; }
    void set_prototype_id(EntityId id) { prototype_id_ = id; }

    /** Teacher/Trainer properties */
    bool is_teacher() const { return is_teacher_; }
    void set_teacher(bool value) { is_teacher_ = value; }
    
    /** Description management */
    std::string_view description() const override { return description_; }
    void set_description(std::string_view desc) override { description_ = desc; }

    /** Article for dynamic name building (nullopt=a/an, ""=none, "the"/"some"=explicit) */
    const std::optional<std::string>& article() const { return article_; }
    void set_article(std::optional<std::string> art) { article_ = std::move(art); }

    /** Base name without article for dynamic name building */
    std::string_view base_name() const { return base_name_; }
    void set_base_name(std::string_view name) { base_name_ = name; }

    /** Override display_name to add effect descriptors (glowing, invisible, etc) */
    std::string display_name(bool with_article = false) const override;

    /** Get effect descriptor for visible effects (sanctuary=glowing, etc) */
    std::string_view effect_descriptor() const;

    /** Memory for received messages (for AI) */
    const std::vector<std::string>& get_received_messages() const { return received_messages_; }
    void clear_received_messages() { received_messages_.clear(); }

    /** Mobile-specific properties loaded from database */
    std::string_view life_force() const { return life_force_; }
    void set_life_force(std::string_view lf) { life_force_ = lf; }

    std::string_view composition() const { return composition_; }
    void set_composition(std::string_view c) { composition_ = c; }

    std::string_view damage_type() const { return damage_type_; }
    void set_damage_type(std::string_view dt) { damage_type_ = dt; }

    /** Combat stats */
    int bare_hand_damage_dice_num() const { return bare_hand_dice_num_; }
    int bare_hand_damage_dice_size() const { return bare_hand_dice_size_; }
    int bare_hand_damage_dice_bonus() const { return bare_hand_dice_bonus_; }
    void set_bare_hand_damage(int num, int size, int bonus = 0) {
        bare_hand_dice_num_ = num;
        bare_hand_dice_size_ = size;
        bare_hand_dice_bonus_ = bonus;
    }

    /** HP dice (used to calculate HP when spawned) */
    int hp_dice_num() const { return hp_dice_num_; }
    int hp_dice_size() const { return hp_dice_size_; }
    int hp_dice_bonus() const { return hp_dice_bonus_; }
    void set_hp_dice(int num, int size, int bonus = 0) {
        hp_dice_num_ = num;
        hp_dice_size_ = size;
        hp_dice_bonus_ = bonus;
    }

    /** Class ID for class-specific guildmasters (0 = no class, matches player's class for training) */
    int class_id() const { return class_id_; }
    void set_class_id(int id) { class_id_ = id; }

    /** Calculate and set HP from dice - returns the calculated max HP */
    int calculate_hp_from_dice();

    /** Mobile traits (what the mob IS - identity) */
    using Actor::set_flag;  // Bring base class ActorFlag version into scope
    bool has_trait(MobTrait trait) const {
        return traits_.test(static_cast<size_t>(trait));
    }
    void set_trait(MobTrait trait, bool value = true) {
        traits_.set(static_cast<size_t>(trait), value);
    }
    void clear_trait(MobTrait trait) { set_trait(trait, false); }

    /** Mobile behaviors (how the mob ACTS) */
    bool has_behavior(MobBehavior behavior) const {
        return behaviors_.test(static_cast<size_t>(behavior));
    }
    void set_behavior(MobBehavior behavior, bool value = true) {
        behaviors_.set(static_cast<size_t>(behavior), value);
        // Sync special role booleans for commonly-used behaviors
        if (behavior == MobBehavior::Teacher) is_teacher_ = value;
    }
    void clear_behavior(MobBehavior behavior) { set_behavior(behavior, false); }

    /** Mobile professions (services the mob provides) */
    bool has_profession(MobProfession profession) const {
        return professions_.test(static_cast<size_t>(profession));
    }
    void set_profession(MobProfession profession, bool value = true) {
        professions_.set(static_cast<size_t>(profession), value);
        // Sync special role booleans for commonly-used professions
        if (profession == MobProfession::Banker) is_banker_ = value;
        if (profession == MobProfession::Receptionist) is_receptionist_ = value;
        if (profession == MobProfession::Postmaster) is_postmaster_ = value;
        if (profession == MobProfession::Shopkeeper) is_shopkeeper_ = value;
    }
    void clear_profession(MobProfession profession) { set_profession(profession, false); }

    /** Effect flags (magical effects on this mob like Invisible, Fly, etc) */
    bool has_effect(EffectFlag effect) const {
        return effect_flags_.contains(effect);
    }
    void set_effect(EffectFlag effect, bool value = true) {
        if (value) {
            effect_flags_.insert(effect);
        } else {
            effect_flags_.erase(effect);
        }
    }
    void remove_effect(EffectFlag effect) { set_effect(effect, false); }
    const std::unordered_set<EffectFlag>& effect_flags() const { return effect_flags_; }

    /** Stance (combat posture: Dead, Sleeping, Resting, Alert, Fighting) */
    db::Stance stance() const { return stance_; }
    void set_stance(db::Stance s) { stance_ = s; }
    void set_stance(std::string_view s);  // Parse from database string

    /** Money carried by this mobile (for loot drops) */
    fiery::Money& money() { return money_; }
    const fiery::Money& money() const { return money_; }
    void set_money(const fiery::Money& m) { money_ = m; }

    /** Take all money from this mobile and return it */
    fiery::Money take_all_money() {
        fiery::Money taken = money_;
        money_ = fiery::Money();
        return taken;
    }

    // Unified Wealth Interface implementation
    long wealth() const override { return money_.value(); }
    void give_wealth(long copper_amount) override {
        if (copper_amount > 0) money_ += fiery::Money(copper_amount);
    }
    bool take_wealth(long copper_amount) override {
        if (copper_amount <= 0 || money_.value() < copper_amount) return false;
        money_ -= fiery::Money(copper_amount);
        return true;
    }
    bool can_afford(long copper_amount) const override {
        return money_.value() >= copper_amount;
    }

protected:
    Mobile(EntityId id, std::string_view name, int level = 1);

private:
    // Reorganized flag system (replaces old mobFlags)
    std::bitset<MOB_TRAIT_CAPACITY> traits_;           // MobTrait - what mob IS
    std::bitset<MOB_BEHAVIOR_CAPACITY> behaviors_;     // MobBehavior - how mob ACTS
    std::bitset<MOB_PROFESSION_CAPACITY> professions_; // MobProfession - services provided
    std::optional<std::string> aggro_condition_;       // Lua expression from database for aggression
    bool is_shopkeeper_ = false;
    bool is_banker_ = false;
    bool is_receptionist_ = false;
    bool is_postmaster_ = false;
    bool is_teacher_ = false;       // Synced with MobBehavior::Teacher
    EntityId prototype_id_;         // For spawned mobs, the prototype they came from
    std::vector<std::string> received_messages_;
    std::string description_ = "";  // Detailed description for NPCs

    // Article and base name for dynamic display
    std::optional<std::string> article_;  // nullopt=a/an, ""=none, "the"/"some"=explicit
    std::string base_name_;               // Name without article for dynamic display

    // Mobile-specific properties from database
    std::string life_force_ = "Life";
    std::string composition_ = "Flesh";
    std::string damage_type_ = "Hit";
    int bare_hand_dice_num_ = 1;
    int bare_hand_dice_size_ = 4;
    int bare_hand_dice_bonus_ = 0;

    // HP dice for calculating HP on spawn
    int hp_dice_num_ = 1;
    int hp_dice_size_ = 8;
    int hp_dice_bonus_ = 0;

    // Class ID for guildmasters (0 = no class, matches player class for training)
    int class_id_ = 0;

    // Combat stance (Dead, Sleeping, Resting, Alert, Fighting)
    db::Stance stance_ = db::Stance::Alert;

    // Effect flags (magical effects like Invisible, Fly, Sanctuary, etc)
    std::unordered_set<EffectFlag> effect_flags_;

    // Money carried by this mobile
    fiery::Money money_;

    void initialize_for_level(int level);
};

/** Learned ability data stored on Player */
struct LearnedAbility {
    int ability_id = 0;
    std::string name;               // Cached ability name
    std::string plain_name;         // Plain text name for lookups
    std::string description;        // Cached description for help system
    bool known = false;             // Has the player learned this?
    int proficiency = 0;            // 0-100 skill percentage
    std::chrono::system_clock::time_point last_used{};  // Last usage time

    // Ability metadata (cached from database)
    std::string type;               // SPELL, SKILL, CHANT, SONG
    int min_level = 1;              // Minimum level to use
    bool violent = false;           // Is this a combat ability?
    int circle = 0;                 // Spell circle (1-9) for spells, 0 for skills
    std::string sphere;             // Spell sphere (fire, water, healing, etc.)
};

/** Color output level preference */
enum class ColorLevel {
    Off = 0,        // No color codes
    Sparse = 1,     // Minimal color (highlights only)
    Normal = 2,     // Standard color usage
    Complete = 3    // Full color everywhere
};

/** Player preference flags */
enum class PlayerFlag {
    // Display preferences
    Brief = 0,          // Show shortened room descriptions
    Compact,            // Reduce blank lines in output
    NoRepeat,           // Don't echo commands back

    // Auto-actions
    AutoLoot,           // Automatically loot corpses after kills
    AutoGold,           // Automatically take gold from corpses
    AutoSplit,          // Automatically split gold with group
    AutoExit,           // Automatically show exits
    AutoAssist,         // Automatically assist group members

    // Combat preferences
    Wimpy,              // Flee when HP is low (threshold stored separately)
    ShowDiceRolls,      // Show detailed dice rolls and damage calculations in combat

    // Social/Status
    Afk,                // Away from keyboard
    Deaf,               // Ignore shouts and gossip
    NoTell,             // Refuse tells from non-gods
    NoSummon,           // Refuse summon spells
    Quest,              // Currently on a quest

    // PK/Consent
    PkEnabled,          // Player killing enabled
    Consent,            // Consent to dangerous actions from other players

    // Misc
    ColorBlind,         // Use colorblind-friendly colors
    Msp,                // MUD Sound Protocol enabled
    MxpEnabled,         // MUD eXtension Protocol enabled

    // Immortal preferences
    HolyLight,          // See everything (invisible, dark, hidden, etc.)
    ShowIds,            // Show entity IDs on mobs/objects/rooms

    // Count for bitset sizing
    MAX_PLAYER_FLAGS
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
    
    /** JSON serialization */
    nlohmann::json to_json() const override;
    
    /** Display name for players returns their actual name */
    std::string display_name(bool with_article = false) const override;
    
    /** Communication (Players queue output to session) */
    void send_message(std::string_view message) override;
    void receive_message(std::string_view message) override;

    /** Death handling - becomes ghost (corpse created on release)
     *  Returns nullptr (player corpse created on release, not death) */
    std::shared_ptr<Container> die() override;
    
    /** Database persistence */
    std::string_view database_id() const { return database_id_; }
    void set_database_id(std::string_view id) { database_id_ = id; }

    /** Player-specific properties */
    std::string_view account() const { return account_; }
    void set_account(std::string_view account) { account_ = account; }

    /** Level management */
    void set_level(int level) { stats().level = std::max(1, level); }
    
    bool is_online() const { return online_; }
    void set_online(bool value) { online_ = value; }
    
    /** Start room (login location) management */
    EntityId start_room() const { return start_room_; }
    void set_start_room(EntityId room_id) { start_room_ = room_id; }

    /** Recall room (touchstone/recall point) management */
    EntityId recall_room() const { return recall_room_; }
    void set_recall_room(EntityId room_id) { recall_room_ = room_id; }
    
    /** Output queue for session handling */
    const std::vector<std::string>& get_output_queue() const { return output_queue_; }
    void clear_output_queue() { output_queue_.clear(); }
    
    /** Player privileges */
    bool is_god() const { return god_level_ > 0; }
    int god_level() const { return god_level_; }
    void set_god_level(int level) { god_level_ = std::max(0, level); }
    
    /** Linkdead state management */
    bool is_linkdead() const { return linkdead_; }
    void set_linkdead(bool linkdead) { linkdead_ = linkdead; }

    /** Composer system - multi-line text input mode */
    void start_composing(std::shared_ptr<ComposerSystem> composer);
    void stop_composing();
    void interrupt_composing(std::string_view reason = "");  // Cancel due to external event
    bool is_composing() const { return active_composer_ != nullptr; }
    std::shared_ptr<ComposerSystem> get_composer() const { return active_composer_; }

    /** GMCP support methods */
    nlohmann::json get_vitals_gmcp() const;
    nlohmann::json get_status_gmcp() const;
    void send_gmcp_vitals_update();
    
    /** Player output interface for network layer */
    void set_output(std::shared_ptr<class PlayerOutput> output) { output_ = output; }
    std::shared_ptr<class PlayerOutput> get_output() const { return output_; }
    
    /** Convenience methods for network layer */
    std::shared_ptr<Room> current_room_ptr() const { return current_room(); }
    int level() const { return stats().level; }
    std::string player_class() const { return player_class_; }

    /** Override race() to return string instead of string_view for compatibility */
    std::string_view race() const override { return Actor::race(); }

    /** Set character class */
    void set_class(std::string_view character_class) {
        player_class_ = character_class;
        initialize_spell_slots();
    }
    
    /** Player title management */
    std::string_view title() const { return title_; }
    void set_title(std::string_view player_title) { title_ = player_title; }

    /** Player description */
    std::string_view description() const override { return description_; }
    void set_description(std::string_view desc) override { description_ = desc; }

    /** Player preference flags */
    bool has_player_flag(PlayerFlag flag) const {
        return player_flags_.test(static_cast<size_t>(flag));
    }
    void set_player_flag(PlayerFlag flag, bool value = true) {
        player_flags_.set(static_cast<size_t>(flag), value);
    }
    void toggle_player_flag(PlayerFlag flag) {
        player_flags_.flip(static_cast<size_t>(flag));
    }
    void clear_player_flag(PlayerFlag flag) { set_player_flag(flag, false); }

    /** Convenience flag accessors */
    bool is_brief() const { return has_player_flag(PlayerFlag::Brief); }
    bool is_compact() const { return has_player_flag(PlayerFlag::Compact); }
    bool is_autoloot() const { return has_player_flag(PlayerFlag::AutoLoot); }
    bool is_autogold() const { return has_player_flag(PlayerFlag::AutoGold); }
    bool is_autosplit() const { return has_player_flag(PlayerFlag::AutoSplit); }
    bool is_autoexit() const { return has_player_flag(PlayerFlag::AutoExit); }
    bool is_afk() const override { return has_player_flag(PlayerFlag::Afk); }
    bool is_deaf() const { return has_player_flag(PlayerFlag::Deaf); }
    bool is_notell() const { return has_player_flag(PlayerFlag::NoTell); }
    bool is_pk_enabled() const { return has_player_flag(PlayerFlag::PkEnabled); }
    bool is_holylight() const override { return has_player_flag(PlayerFlag::HolyLight); }
    bool is_show_ids() const override { return has_player_flag(PlayerFlag::ShowIds); }
    bool is_show_dice_rolls() const { return has_player_flag(PlayerFlag::ShowDiceRolls); }

    /** AFK message (displayed to people who try to interact) */
    std::string_view afk_message() const { return afk_message_; }
    void set_afk_message(std::string_view msg) { afk_message_ = msg; }

    /** Prompt format string - used for command prompt display */
    std::string_view prompt() const { return prompt_; }
    void set_prompt(std::string_view prompt_format) { prompt_ = prompt_format; }

    /** Color level preference */
    ColorLevel color_level() const { return color_level_; }
    void set_color_level(ColorLevel level) { color_level_ = level; }

    /** Wimpy threshold - flee when HP falls below this value */
    int wimpy_threshold() const { return wimpy_threshold_; }
    void set_wimpy_threshold(int threshold) {
        wimpy_threshold_ = std::max(0, threshold);
        set_player_flag(PlayerFlag::Wimpy, wimpy_threshold_ > 0);
    }

    /** Get player flags as strings (for database persistence) */
    std::vector<std::string> get_player_flags_as_strings() const;

    /** Set player flags from strings (from database load) */
    void set_player_flags_from_strings(const std::vector<std::string>& flags);

    // =========================================================================
    // Currency (Money-based API)
    // =========================================================================

    /** Get wallet (money on person) */
    fiery::Money& wallet() { return wallet_; }
    const fiery::Money& wallet() const { return wallet_; }

    /** Set wallet */
    void set_wallet(const fiery::Money& money) { wallet_ = money; }

    /** Get bank account */
    fiery::Money& bank() { return bank_; }
    const fiery::Money& bank() const { return bank_; }

    /** Set bank account */
    void set_bank(const fiery::Money& money) { bank_ = money; }

    /** Check if wallet can afford an amount */
    bool can_afford(const fiery::Money& cost) const { return wallet_.can_afford(cost); }
    bool can_afford(long copper_cost) const override { return wallet_.can_afford(copper_cost); }

    // Unified Wealth Interface implementation (uses wallet)
    long wealth() const override { return wallet_.value(); }
    void give_wealth(long copper_amount) override {
        if (copper_amount > 0) wallet_.receive(copper_amount);
    }
    bool take_wealth(long copper_amount) override {
        return spend(copper_amount);
    }

    /** Check if bank can afford an amount */
    bool bank_can_afford(const fiery::Money& cost) const { return bank_.can_afford(cost); }
    bool bank_can_afford(long copper_cost) const { return bank_.can_afford(copper_cost); }

    /** Spend from wallet (uses highest denominations first, makes change)
     *  Returns true if successful, false if insufficient funds */
    bool spend(const fiery::Money& cost) { return wallet_.charge(cost); }
    bool spend(long copper_cost) { return wallet_.charge(copper_cost); }

    /** Spend from bank
     *  Returns true if successful, false if insufficient funds */
    bool spend_from_bank(const fiery::Money& cost) { return bank_.charge(cost); }
    bool spend_from_bank(long copper_cost) { return bank_.charge(copper_cost); }

    /** Receive money into wallet */
    void receive(const fiery::Money& money) { wallet_ += money; }
    void receive(long copper_amount) { wallet_.receive(copper_amount); }

    /** Deposit from wallet to bank */
    bool deposit(const fiery::Money& amount) {
        if (!wallet_.charge(amount)) return false;
        bank_ += amount;
        return true;
    }

    /** Withdraw from bank to wallet */
    bool withdraw(const fiery::Money& amount) {
        if (!bank_.charge(amount)) return false;
        wallet_ += amount;
        return true;
    }

    // =========================================================================
    // Account Bank (shared across all characters on account)
    // =========================================================================

    /** Get account bank (shared wealth accessible by all characters on account) */
    fiery::Money& account_bank() { return account_bank_; }
    const fiery::Money& account_bank() const { return account_bank_; }

    /** Set account bank */
    void set_account_bank(const fiery::Money& money) { account_bank_ = money; }

    /** Check if account bank can afford an amount */
    bool account_can_afford(const fiery::Money& cost) const { return account_bank_.can_afford(cost); }
    bool account_can_afford(long copper_cost) const { return account_bank_.can_afford(copper_cost); }

    /** Deposit from wallet to account bank */
    bool account_deposit(const fiery::Money& amount) {
        if (!wallet_.charge(amount)) return false;
        account_bank_ += amount;
        return true;
    }

    /** Withdraw from account bank to wallet */
    bool account_withdraw(const fiery::Money& amount) {
        if (!account_bank_.charge(amount)) return false;
        wallet_ += amount;
        return true;
    }

    /** User account link for database persistence */
    std::string_view user_id() const { return user_id_; }
    void set_user_id(std::string_view id) { user_id_ = id; }
    bool has_user_account() const { return !user_id_.empty(); }

    /** Transfer money to another player */
    bool transfer_to(Player& recipient, const fiery::Money& amount) {
        if (!wallet_.charge(amount)) return false;
        recipient.wallet() += amount;
        return true;
    }

    /** Take all money from a Mobile (for loot) */
    fiery::Money loot_money_from(class Mobile& mob);

    /** Time tracking */
    std::time_t creation_time() const { return creation_time_; }
    std::time_t last_logon_time() const { return last_logon_time_; }
    std::chrono::seconds total_play_time() const { return total_play_time_; }
    void set_last_logon(std::time_t time) { last_logon_time_ = time; }
    void add_play_time(std::chrono::seconds time) { total_play_time_ += time; }

    /** PK mode timing */
    std::time_t pk_enabled_time() const { return pk_enabled_time_; }
    void set_pk_enabled_time(std::time_t time) { pk_enabled_time_ = time; }
    bool can_disable_pk() const {
        if (pk_enabled_time_ == 0) return true;
        constexpr std::time_t SECONDS_IN_24_HOURS = 24 * 60 * 60;
        return (std::time(nullptr) - pk_enabled_time_) >= SECONDS_IN_24_HOURS;
    }

    /** Clan/Guild membership */
    bool in_clan() const { return clan_id_ > 0; }
    unsigned int clan_id() const { return clan_id_; }
    int clan_rank_index() const { return clan_rank_index_; }
    std::string_view clan_name() const { return clan_name_; }
    std::string_view clan_abbreviation() const { return clan_abbreviation_; }
    std::string_view clan_rank_title() const { return clan_rank_title_; }
    void set_clan(unsigned int clan_id, std::string_view name, std::string_view abbreviation,
                  int rank_index, std::string_view rank_title) {
        clan_id_ = clan_id;
        clan_name_ = name;
        clan_abbreviation_ = abbreviation;
        clan_rank_index_ = rank_index;
        clan_rank_title_ = rank_title;
    }
    void clear_clan() {
        clan_id_ = 0;
        clan_rank_index_ = -1;
        clan_name_.clear();
        clan_abbreviation_.clear();
        clan_rank_title_.clear();
    }

    /** Initialize spell slots based on class and level */
    void initialize_spell_slots();
    
protected:
    Player(EntityId id, std::string_view name);
    
    /** Room change notification - sends GMCP room updates */
    void on_room_change(std::shared_ptr<Room> old_room, std::shared_ptr<Room> new_room) override;

    /** Position change notification - sends GMCP vitals updates */
    void on_position_change(Position old_pos, Position new_pos) override;

    /** Level up notification - sends GMCP vitals updates */
    void on_level_up(int old_level, int new_level) override;
    
private:
    std::string database_id_;  // UUID from database for persistence
    std::string account_;
    bool online_ = false;
    bool linkdead_ = false;  // Player connection lost but still in world
    int god_level_ = 0;
    std::shared_ptr<ComposerSystem> active_composer_;  // Multi-line text composer
    EntityId start_room_ = INVALID_ENTITY_ID;  // Player's login location (where they logged out)
    EntityId recall_room_ = INVALID_ENTITY_ID; // Player's recall/touchstone location
    std::vector<std::string> output_queue_;
    std::shared_ptr<class PlayerOutput> output_;
    
    // Character creation fields
    std::string player_class_ = "warrior";  // Default class
    std::string title_ = "";                // Player title
    std::string description_ = "";          // Player description

    // Player preferences
    std::bitset<static_cast<size_t>(PlayerFlag::MAX_PLAYER_FLAGS)> player_flags_;
    int wimpy_threshold_ = 0;               // Flee when HP falls below this
    std::string afk_message_ = "";          // AFK message to display
    std::string prompt_ = "<%h/%Hhp %v/%Vs>";  // Prompt format string
    ColorLevel color_level_ = ColorLevel::Normal;  // Color output preference

    // Currency
    fiery::Money wallet_;        // Money on person
    fiery::Money bank_;          // Money in character's bank account
    fiery::Money account_bank_;  // Money in shared account bank (all characters)
    std::string user_id_;        // UUID of linked user account (for account bank persistence)

    // Time tracking fields
    std::time_t creation_time_ = std::time(nullptr);  // Time of character creation
    std::time_t last_logon_time_ = 0;                 // Time of last logon
    std::chrono::seconds total_play_time_{0};         // Total accumulated play time
    std::time_t pk_enabled_time_ = 0;                 // When PK mode was enabled (0 = never)

    // Clan/Guild membership
    unsigned int clan_id_ = 0;               // Clan ID (0 = no clan)
    int clan_rank_index_ = -1;               // Rank within clan (-1 = not in clan)
    std::string clan_name_;                  // Cached clan name for display
    std::string clan_abbreviation_;          // Cached clan abbreviation
    std::string clan_rank_title_;            // Cached rank title for display

    // Communication tracking
    std::string last_tell_sender_;           // Name of last player who sent us a tell
    std::vector<std::string> tell_history_;  // Recent tell history
    static constexpr size_t MAX_TELL_HISTORY = 20;

    // Text composer recovery (for 'redo' command)
    std::vector<std::string> redo_buffer_;   // Last interrupted text composition

    // Ability/Skill system
    std::unordered_map<int, LearnedAbility> abilities_;  // ability_id -> learned ability data

    // Command aliases
    std::unordered_map<std::string, std::string> aliases_;  // alias -> command

    // Ignored players (by character name, lowercase)
    std::unordered_set<std::string> ignored_players_;

    // Group/follow system
    std::weak_ptr<Actor> leader_;            // Who we are following (empty = we are leader or solo)
    std::vector<std::weak_ptr<Actor>> followers_; // Who is following us
    bool group_flag_ = true;                 // Whether we accept group invites

    // Meditation state
    bool is_meditating_ = false;             // Currently meditating for spell restoration

public:
    // Meditation methods (increases effective focus for spell slot restoration)
    bool is_meditating() const { return is_meditating_; }
    void start_meditation();
    void stop_meditation();

    /**
     * Get effective focus for spell slot restoration.
     * Meditation doubles the focus rate.
     */
    int get_spell_restore_rate() const;

    // Communication tracking methods
    void set_last_tell_sender(std::string_view sender) { last_tell_sender_ = sender; }
    std::string_view last_tell_sender() const { return last_tell_sender_; }

    void add_tell_to_history(std::string_view entry) {
        tell_history_.push_back(std::string(entry));
        if (tell_history_.size() > MAX_TELL_HISTORY) {
            tell_history_.erase(tell_history_.begin());
        }
    }
    const std::vector<std::string>& tell_history() const { return tell_history_; }
    void clear_tell_history() { tell_history_.clear(); }

    // Text composer redo buffer methods
    void set_redo_buffer(const std::vector<std::string>& lines) { redo_buffer_ = lines; }
    void set_redo_buffer(std::vector<std::string>&& lines) { redo_buffer_ = std::move(lines); }
    const std::vector<std::string>& redo_buffer() const { return redo_buffer_; }
    void clear_redo_buffer() { redo_buffer_.clear(); }
    bool has_redo_buffer() const { return !redo_buffer_.empty(); }

    // Group/follow system methods
    bool has_group() const {
        return !leader_.expired() || !followers_.empty();
    }

    bool is_group_leader() const {
        return leader_.expired() && !followers_.empty();
    }

    bool is_following() const {
        return !leader_.expired();
    }

    std::shared_ptr<Actor> get_leader() const {
        return leader_.lock();
    }

    const std::vector<std::weak_ptr<Actor>>& get_followers() const {
        return followers_;
    }

    void set_leader(std::shared_ptr<Actor> leader) {
        leader_ = leader;
    }

    void clear_leader() {
        leader_.reset();
    }

    void add_follower(std::shared_ptr<Actor> follower) {
        // Clean up any expired weak pointers first
        followers_.erase(
            std::remove_if(followers_.begin(), followers_.end(),
                [](const std::weak_ptr<Actor>& wp) { return wp.expired(); }),
            followers_.end());
        followers_.push_back(follower);
    }

    void remove_follower(std::shared_ptr<Actor> follower) {
        followers_.erase(
            std::remove_if(followers_.begin(), followers_.end(),
                [&follower](const std::weak_ptr<Actor>& wp) {
                    auto sp = wp.lock();
                    return !sp || sp == follower;
                }),
            followers_.end());
    }

    bool group_flag() const { return group_flag_; }
    void set_group_flag(bool value) { group_flag_ = value; }

    void send_to_group(std::string_view message);

    // ============================================================================
    // Ability/Skill System
    // ============================================================================

    /** Check if player has learned an ability (at any proficiency) */
    bool has_ability(int ability_id) const {
        auto it = abilities_.find(ability_id);
        return it != abilities_.end() && it->second.known;
    }

    /** Get a specific learned ability (nullptr if not found) */
    const LearnedAbility* get_ability(int ability_id) const {
        auto it = abilities_.find(ability_id);
        return (it != abilities_.end()) ? &it->second : nullptr;
    }

    /** Get mutable ability (nullptr if not found) */
    LearnedAbility* get_ability_mutable(int ability_id) {
        auto it = abilities_.find(ability_id);
        return (it != abilities_.end()) ? &it->second : nullptr;
    }

    /** Get all learned abilities (const reference) */
    const std::unordered_map<int, LearnedAbility>& get_abilities() const {
        return abilities_;
    }

    /** Add or update a learned ability */
    void set_ability(const LearnedAbility& ability) {
        abilities_[ability.ability_id] = ability;
    }

    /** Get proficiency for a specific ability (0 if not learned) */
    int get_proficiency(int ability_id) const {
        auto it = abilities_.find(ability_id);
        return (it != abilities_.end() && it->second.known) ? it->second.proficiency : 0;
    }

    /** Set proficiency for an ability (must already exist) */
    bool set_proficiency(int ability_id, int proficiency) {
        auto it = abilities_.find(ability_id);
        if (it != abilities_.end()) {
            it->second.proficiency = std::clamp(proficiency, 0, 100);
            return true;
        }
        return false;
    }

    /** Mark ability as known/learned */
    bool learn_ability(int ability_id) {
        auto it = abilities_.find(ability_id);
        if (it != abilities_.end()) {
            it->second.known = true;
            return true;
        }
        return false;
    }

    /** Get abilities filtered by type (SPELL, SKILL, CHANT, SONG) */
    std::vector<const LearnedAbility*> get_abilities_by_type(std::string_view type) const {
        std::vector<const LearnedAbility*> result;
        for (const auto& [id, ability] : abilities_) {
            if (ability.type == type) {
                result.push_back(&ability);
            }
        }
        return result;
    }

    /** Get all known abilities (learned with proficiency > 0) */
    std::vector<const LearnedAbility*> get_known_abilities() const {
        std::vector<const LearnedAbility*> result;
        for (const auto& [id, ability] : abilities_) {
            if (ability.known) {
                result.push_back(&ability);
            }
        }
        return result;
    }

    /** Find an ability by name (partial match, case-insensitive) */
    const LearnedAbility* find_ability_by_name(std::string_view search) const {
        std::string search_lower{search};
        std::transform(search_lower.begin(), search_lower.end(), search_lower.begin(), ::tolower);

        for (const auto& [id, ability] : abilities_) {
            std::string name_lower = ability.name;
            std::transform(name_lower.begin(), name_lower.end(), name_lower.begin(), ::tolower);
            std::string plain_lower = ability.plain_name;
            std::transform(plain_lower.begin(), plain_lower.end(), plain_lower.begin(), ::tolower);

            // Match from beginning of either name format
            if (name_lower.find(search_lower) == 0 || plain_lower.find(search_lower) == 0) {
                return &ability;
            }
        }
        return nullptr;
    }

    /** Record ability usage time */
    void record_ability_use(int ability_id) {
        auto it = abilities_.find(ability_id);
        if (it != abilities_.end()) {
            it->second.last_used = std::chrono::system_clock::now();
        }
    }

    /** Clear all abilities (for reloading from database) */
    void clear_abilities() {
        abilities_.clear();
    }

    // ============================================================================
    // Alias System
    // ============================================================================

    /** Get all aliases */
    const std::unordered_map<std::string, std::string>& get_aliases() const {
        return aliases_;
    }

    /** Set an alias (creates or updates) */
    void set_alias(std::string_view alias, std::string_view command) {
        aliases_[std::string(alias)] = std::string(command);
    }

    /** Remove an alias */
    bool remove_alias(std::string_view alias) {
        return aliases_.erase(std::string(alias)) > 0;
    }

    /** Check if an alias exists */
    bool has_alias(std::string_view alias) const {
        return aliases_.find(std::string(alias)) != aliases_.end();
    }

    /** Get the command for an alias (returns nullopt if not found) */
    std::optional<std::string_view> get_alias(std::string_view alias) const {
        auto it = aliases_.find(std::string(alias));
        if (it != aliases_.end()) {
            return it->second;
        }
        return std::nullopt;
    }

    /** Clear all aliases */
    void clear_aliases() {
        aliases_.clear();
    }

    // ============================================================================
    // Ignored Players System
    // ============================================================================

    /** Ignore a player (stores lowercase name) */
    void ignore_player(std::string_view player_name) {
        std::string name_lower{player_name};
        std::transform(name_lower.begin(), name_lower.end(), name_lower.begin(), ::tolower);
        ignored_players_.insert(name_lower);
    }

    /** Stop ignoring a player */
    bool unignore_player(std::string_view player_name) {
        std::string name_lower{player_name};
        std::transform(name_lower.begin(), name_lower.end(), name_lower.begin(), ::tolower);
        return ignored_players_.erase(name_lower) > 0;
    }

    /** Check if ignoring a player */
    bool is_ignoring(std::string_view player_name) const {
        std::string name_lower{player_name};
        std::transform(name_lower.begin(), name_lower.end(), name_lower.begin(), ::tolower);
        return ignored_players_.find(name_lower) != ignored_players_.end();
    }

    /** Get all ignored players */
    const std::unordered_set<std::string>& get_ignored_players() const {
        return ignored_players_;
    }

    /** Clear all ignored players */
    void clear_ignored_players() {
        ignored_players_.clear();
    }
};

/** Actor utility functions */
namespace ActorUtils {
    /** Calculate experience required for level */
    long experience_for_level(int level);
    
    /** Calculate hit points for level and constitution */
    int calculate_hit_points(int level, int constitution);
    
    /** Calculate mana for level and intelligence */
    int calculate_mana(int level, int intelligence);
    
    /** Calculate stamina for level and constitution */
    int calculate_stamina(int level, int constitution);
    
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