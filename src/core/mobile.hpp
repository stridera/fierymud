#pragma once

#include "actor.hpp"

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

