#pragma once

#include "entity.hpp"
#include "object.hpp"
#include "active_effect.hpp"
#include "core/result.hpp"

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
