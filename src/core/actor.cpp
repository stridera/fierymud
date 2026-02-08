#include "actor.hpp"
#include "mobile.hpp"
#include "player.hpp"
#include "object.hpp"
#include "spell_system.hpp"
#include "core/logging.hpp"
#include "game/composer_system.hpp"
#include "game/player_output.hpp"
#include "text/string_utils.hpp"
#include "world/room.hpp"
#include "world/world_manager.hpp"
#include "net/player_connection.hpp"
#include "database/generated/db_mob.hpp"

#include <magic_enum/magic_enum.hpp>
#include <nlohmann/json.hpp>
#include <algorithm>
#include <cmath>
#include <sstream>

// Gameplay constants
namespace {
    // Attribute bounds
    constexpr int MIN_ATTRIBUTE = 1;
    constexpr int MAX_ATTRIBUTE = 25;

    // Level bounds (MAX_LEVEL supports immortal levels above 100)
    constexpr int MIN_LEVEL = 1;
    constexpr int MAX_LEVEL = 200;  // Immortals can be level 101-200

    // Carry capacity calculation
    constexpr int BASE_CARRY_CAPACITY = 50;
    constexpr int CARRY_CAPACITY_PER_STR_MOD = 10;

    // Money conversion rates (copper as base)
    constexpr int SILVER_TO_COPPER = 10;
    constexpr int GOLD_TO_COPPER = 100;
    constexpr int PLATINUM_TO_COPPER = 1000;

    // Message queue limits
    constexpr size_t MAX_RECEIVED_MESSAGES = 10;

    // Experience calculation constants
    constexpr double EXP_LEVEL_EXPONENT = 2.5;
    constexpr int EXP_BASE_MULTIPLIER = 1000;

    // Hit points calculation
    constexpr int HP_BASE_LEVEL_MULTIPLIER = 2;
    constexpr int HP_BASE_START = 100;
    constexpr int HP_MINIMUM = 50;

    // Player age base (cosmetic)
    constexpr int PLAYER_BASE_AGE = 20;

    // Time conversions
    constexpr int SECONDS_PER_HOUR = 3600;
}

// Stats Implementation

Result<void> Stats::validate() const {
    if (strength < MIN_ATTRIBUTE || strength > MAX_ATTRIBUTE) {
        return std::unexpected(Errors::InvalidState(
            fmt::format("Strength must be between {} and {}", MIN_ATTRIBUTE, MAX_ATTRIBUTE)));
    }
    if (dexterity < MIN_ATTRIBUTE || dexterity > MAX_ATTRIBUTE) {
        return std::unexpected(Errors::InvalidState(
            fmt::format("Dexterity must be between {} and {}", MIN_ATTRIBUTE, MAX_ATTRIBUTE)));
    }
    if (intelligence < MIN_ATTRIBUTE || intelligence > MAX_ATTRIBUTE) {
        return std::unexpected(Errors::InvalidState(
            fmt::format("Intelligence must be between {} and {}", MIN_ATTRIBUTE, MAX_ATTRIBUTE)));
    }
    if (wisdom < MIN_ATTRIBUTE || wisdom > MAX_ATTRIBUTE) {
        return std::unexpected(Errors::InvalidState(
            fmt::format("Wisdom must be between {} and {}", MIN_ATTRIBUTE, MAX_ATTRIBUTE)));
    }
    if (constitution < MIN_ATTRIBUTE || constitution > MAX_ATTRIBUTE) {
        return std::unexpected(Errors::InvalidState(
            fmt::format("Constitution must be between {} and {}", MIN_ATTRIBUTE, MAX_ATTRIBUTE)));
    }
    if (charisma < MIN_ATTRIBUTE || charisma > MAX_ATTRIBUTE) {
        return std::unexpected(Errors::InvalidState(
            fmt::format("Charisma must be between {} and {}", MIN_ATTRIBUTE, MAX_ATTRIBUTE)));
    }

    if (hit_points < 0 || hit_points > max_hit_points) {
        return std::unexpected(Errors::InvalidState("Hit points invalid"));
    }
    if (mana < 0 || mana > max_mana) {
        return std::unexpected(Errors::InvalidState("Mana invalid"));
    }
    if (stamina < 0 || stamina > max_stamina) {
        return std::unexpected(Errors::InvalidState("Stamina invalid"));
    }

    if (level < MIN_LEVEL || level > MAX_LEVEL) {
        return std::unexpected(Errors::InvalidState(
            fmt::format("Level must be between {} and {}", MIN_LEVEL, MAX_LEVEL)));
    }
    if (experience < 0) {
        return std::unexpected(Errors::InvalidState("Experience cannot be negative"));
    }
    if (gold < 0) {
        return std::unexpected(Errors::InvalidState("Gold cannot be negative"));
    }

    return Success();
}

int Stats::attribute_modifier(int attribute) {
    if (attribute <= 3) return -4;
    if (attribute <= 5) return -3;
    if (attribute <= 7) return -2;
    if (attribute <= 9) return -1;
    if (attribute <= 11) return 0;
    if (attribute <= 13) return 1;
    if (attribute <= 15) return 2;
    if (attribute <= 17) return 3;
    if (attribute <= 19) return 4;
    return 5; // 20+
}

long Stats::experience_to_next_level() const {
    return ActorUtils::experience_for_level(level + 1) - experience;
}

bool Stats::can_level_up() const {
    return experience >= ActorUtils::experience_for_level(level + 1);
}

nlohmann::json Stats::to_json() const {
    return {
        {"strength", strength},
        {"dexterity", dexterity},
        {"intelligence", intelligence},
        {"wisdom", wisdom},
        {"constitution", constitution},
        {"charisma", charisma},
        {"hit_points", hit_points},
        {"max_hit_points", max_hit_points},
        {"mana", mana},
        {"max_mana", max_mana},
        {"stamina", stamina},
        {"max_stamina", max_stamina},
        // New combat stats
        {"accuracy", accuracy},
        {"attack_power", attack_power},
        {"spell_power", spell_power},
        {"penetration_flat", penetration_flat},
        {"penetration_percent", penetration_percent},
        {"evasion", evasion},
        {"armor_rating", armor_rating},
        {"damage_reduction_percent", damage_reduction_percent},
        {"soak", soak},
        {"hardness", hardness},
        {"ward_percent", ward_percent},
        // Elemental resistances
        {"resistance_fire", resistance_fire},
        {"resistance_cold", resistance_cold},
        {"resistance_lightning", resistance_lightning},
        {"resistance_acid", resistance_acid},
        {"resistance_poison", resistance_poison},
        {"level", level},
        {"experience", experience},
        {"gold", gold},
        {"alignment", alignment}
    };
}

Result<Stats> Stats::from_json(const nlohmann::json& json) {
    try {
        Stats stats;

        if (json.contains("strength")) stats.strength = json["strength"].get<int>();
        if (json.contains("dexterity")) stats.dexterity = json["dexterity"].get<int>();
        if (json.contains("intelligence")) stats.intelligence = json["intelligence"].get<int>();
        if (json.contains("wisdom")) stats.wisdom = json["wisdom"].get<int>();
        if (json.contains("constitution")) stats.constitution = json["constitution"].get<int>();
        if (json.contains("charisma")) stats.charisma = json["charisma"].get<int>();

        if (json.contains("hit_points")) stats.hit_points = json["hit_points"].get<int>();
        if (json.contains("max_hit_points")) stats.max_hit_points = json["max_hit_points"].get<int>();
        if (json.contains("mana")) stats.mana = json["mana"].get<int>();
        if (json.contains("max_mana")) stats.max_mana = json["max_mana"].get<int>();
        if (json.contains("stamina")) stats.stamina = json["stamina"].get<int>();
        if (json.contains("max_stamina")) stats.max_stamina = json["max_stamina"].get<int>();

        // New combat stats
        if (json.contains("accuracy")) stats.accuracy = json["accuracy"].get<int>();
        if (json.contains("attack_power")) stats.attack_power = json["attack_power"].get<int>();
        if (json.contains("spell_power")) stats.spell_power = json["spell_power"].get<int>();
        if (json.contains("penetration_flat")) stats.penetration_flat = json["penetration_flat"].get<int>();
        if (json.contains("penetration_percent")) stats.penetration_percent = json["penetration_percent"].get<int>();
        if (json.contains("evasion")) stats.evasion = json["evasion"].get<int>();
        if (json.contains("armor_rating")) stats.armor_rating = json["armor_rating"].get<int>();
        if (json.contains("damage_reduction_percent")) stats.damage_reduction_percent = json["damage_reduction_percent"].get<int>();
        if (json.contains("soak")) stats.soak = json["soak"].get<int>();
        if (json.contains("hardness")) stats.hardness = json["hardness"].get<int>();
        if (json.contains("ward_percent")) stats.ward_percent = json["ward_percent"].get<int>();

        // Elemental resistances
        if (json.contains("resistance_fire")) stats.resistance_fire = json["resistance_fire"].get<int>();
        if (json.contains("resistance_cold")) stats.resistance_cold = json["resistance_cold"].get<int>();
        if (json.contains("resistance_lightning")) stats.resistance_lightning = json["resistance_lightning"].get<int>();
        if (json.contains("resistance_acid")) stats.resistance_acid = json["resistance_acid"].get<int>();
        if (json.contains("resistance_poison")) stats.resistance_poison = json["resistance_poison"].get<int>();

        if (json.contains("level")) stats.level = json["level"].get<int>();
        if (json.contains("experience")) stats.experience = json["experience"].get<long>();
        if (json.contains("gold")) stats.gold = json["gold"].get<long>();
        if (json.contains("alignment")) stats.alignment = json["alignment"].get<int>();

        TRY(stats.validate());

        return stats;

    } catch (const nlohmann::json::exception& e) {
        return std::unexpected(Errors::ParseError("Stats JSON parsing error", e.what()));
    }
}

// Inventory Implementation

Result<void> Inventory::add_item(std::shared_ptr<Object> item) {
    if (!item) {
        return std::unexpected(Errors::InvalidArgument("item", "cannot be null"));
    }

    items_.push_back(std::move(item));
    return Success();
}

std::shared_ptr<Object> Inventory::remove_item(EntityId item_id) {
    auto it = std::find_if(items_.begin(), items_.end(),
        [item_id](const std::shared_ptr<Object>& item) {
            return item && item->id() == item_id;
        });

    if (it != items_.end()) {
        auto item = *it;
        items_.erase(it);
        return item;
    }

    return nullptr;
}

bool Inventory::remove_item(const std::shared_ptr<Object>& item) {
    auto it = std::find(items_.begin(), items_.end(), item);
    if (it != items_.end()) {
        items_.erase(it);
        return true;
    }
    return false;
}

std::shared_ptr<Object> Inventory::find_item(EntityId item_id) const {
    auto it = std::find_if(items_.begin(), items_.end(),
        [item_id](const std::shared_ptr<Object>& item) {
            return item && item->id() == item_id;
        });

    return it != items_.end() ? *it : nullptr;
}

std::vector<std::shared_ptr<Object>> Inventory::find_items_by_keyword(std::string_view keyword) const {
    std::vector<std::shared_ptr<Object>> matches;

    for (const auto& item : items_) {
        if (item && item->matches_target_string(keyword)) {
            matches.push_back(item);
        }
    }

    return matches;
}

std::span<const std::shared_ptr<Object>> Inventory::get_all_items() const {
    return items_;
}

int Inventory::total_weight() const {
    int weight = 0;
    for (const auto& item : items_) {
        if (item) {
            weight += item->weight();
        }
    }
    return weight;
}

int Inventory::total_value() const {
    int value = 0;
    for (const auto& item : items_) {
        if (item) {
            value += item->value();
        }
    }
    return value;
}

std::vector<std::shared_ptr<Object>> Inventory::clear() {
    std::vector<std::shared_ptr<Object>> removed = std::move(items_);
    items_.clear();
    return removed;
}

bool Inventory::can_carry(int additional_weight, int max_carry_weight) const {
    return total_weight() + additional_weight <= max_carry_weight;
}

nlohmann::json Inventory::to_json() const {
    nlohmann::json json;
    json["items"] = nlohmann::json::array();

    for (const auto& item : items_) {
        if (item) {
            json["items"].push_back(item->to_json());
        }
    }

    return json;
}

Result<Inventory> Inventory::from_json(const nlohmann::json& json) {
    try {
        Inventory inventory;

        if (json.contains("items") && json["items"].is_array()) {
            for (const auto& item_json : json["items"]) {
                auto item_result = Object::from_json(item_json);
                if (item_result) {
                    TRY(inventory.add_item(std::move(item_result.value())));
                } else {
                    Log::error("Failed to load inventory item: {}", item_result.error().message);
                }
            }
        }

        return inventory;

    } catch (const nlohmann::json::exception& e) {
        return std::unexpected(Errors::ParseError("Inventory JSON parsing error", e.what()));
    }
}

// Equipment Implementation

std::string Equipment::get_slot_conflict_message(EquipSlot slot, const Object& item) const {
    // First check: Is this slot already occupied?
    if (is_equipped(slot)) {
        switch (slot) {
            case EquipSlot::Light:      return "You are already using something as light.";
            case EquipSlot::Finger_R:   return "You are already wearing something on your right finger.";
            case EquipSlot::Finger_L:   return "You are already wearing something on your left finger.";
            case EquipSlot::Neck1:
            case EquipSlot::Neck2:      return "You are already wearing something around your neck.";
            case EquipSlot::Body:       return "You are already wearing something on your body.";
            case EquipSlot::Head:       return "You are already wearing something on your head.";
            case EquipSlot::Legs:       return "You are already wearing something on your legs.";
            case EquipSlot::Feet:       return "You are already wearing something on your feet.";
            case EquipSlot::Hands:      return "You are already wearing something on your hands.";
            case EquipSlot::Arms:       return "You are already wearing something on your arms.";
            case EquipSlot::Shield:     return "You are already using something as a shield.";
            case EquipSlot::About:      return "You are already wearing something about your body.";
            case EquipSlot::Waist:      return "You are already wearing something around your waist.";
            case EquipSlot::Wrist_R:    return "You are already wearing something on your right wrist.";
            case EquipSlot::Wrist_L:    return "You are already wearing something on your left wrist.";
            case EquipSlot::Wield:      return "You are already wielding something.";
            case EquipSlot::Hold:       return "You are already holding something.";
            case EquipSlot::Float:      return "You already have something floating nearby.";
            default:                    return "You are already wearing something there.";
        }
    }

    // Check for two-handed weapon conflicts
    if (item.has_flag(ObjectFlag::TwoHanded) && slot == EquipSlot::Wield) {
        if (is_equipped(EquipSlot::Wield2)) {
            return "You can't wield a two-handed weapon while holding something in your off-hand.";
        }
        if (is_equipped(EquipSlot::Shield)) {
            return "You can't wield a two-handed weapon while using a shield.";
        }
    }

    if (slot == EquipSlot::Wield2 || slot == EquipSlot::Shield) {
        auto main_weapon = get_equipped(EquipSlot::Wield);
        if (main_weapon && main_weapon->has_flag(ObjectFlag::TwoHanded)) {
            return "You can't use that while wielding a two-handed weapon.";
        }
    }

    return ""; // No conflict
}

Result<void> Equipment::equip_item(std::shared_ptr<Object> item) {
    if (!item) {
        return std::unexpected(Errors::InvalidArgument("item", "cannot be null"));
    }

    if (!item->is_wearable()) {
        return std::unexpected(Errors::InvalidArgument("item", "is not wearable"));
    }

    EquipSlot slot = item->equip_slot();
    std::string conflict_message = get_slot_conflict_message(slot, *item);
    if (!conflict_message.empty()) {
        return std::unexpected(Errors::InvalidState(conflict_message));
    }

    equipped_[slot] = std::move(item);
    return Success();
}

Result<void> Equipment::equip_to_slot(std::shared_ptr<Object> item, EquipSlot slot) {
    if (!item) {
        return std::unexpected(Errors::InvalidArgument("item", "cannot be null"));
    }

    if (slot == EquipSlot::None) {
        return std::unexpected(Errors::InvalidArgument("slot", "cannot be None"));
    }

    // For loading from database, we trust the saved slot and don't check conflicts
    // The item may have different equip_slot() than where it was saved
    equipped_[slot] = std::move(item);
    return Success();
}

std::shared_ptr<Object> Equipment::unequip_item(EquipSlot slot) {
    auto it = equipped_.find(slot);
    if (it != equipped_.end()) {
        auto item = it->second;
        equipped_.erase(it);
        // Reset two-handed grip when unequipping main weapon
        if (slot == EquipSlot::Wield) {
            reset_grip();
        }
        return item;
    }
    return nullptr;
}

std::shared_ptr<Object> Equipment::unequip_item(EntityId item_id) {
    for (auto it = equipped_.begin(); it != equipped_.end(); ++it) {
        if (it->second && it->second->id() == item_id) {
            auto item = it->second;
            auto slot = it->first;
            equipped_.erase(it);
            // Reset two-handed grip when unequipping main weapon
            if (slot == EquipSlot::Wield) {
                reset_grip();
            }
            return item;
        }
    }
    return nullptr;
}

std::shared_ptr<Object> Equipment::get_equipped(EquipSlot slot) const {
    auto it = equipped_.find(slot);
    return it != equipped_.end() ? it->second : nullptr;
}

bool Equipment::is_equipped(EquipSlot slot) const {
    return equipped_.contains(slot);
}

std::vector<std::shared_ptr<Object>> Equipment::get_all_equipped() const {
    std::vector<std::shared_ptr<Object>> items;
    items.reserve(equipped_.size());

    for (const auto& [slot, item] : equipped_) {
        if (item) {
            items.push_back(item);
        }
    }

    return items;
}

std::vector<std::pair<EquipSlot, std::shared_ptr<Object>>> Equipment::get_all_equipped_with_slots() const {
    std::vector<std::pair<EquipSlot, std::shared_ptr<Object>>> items;
    items.reserve(equipped_.size());

    for (const auto& [slot, item] : equipped_) {
        if (item) {
            items.emplace_back(slot, item);
        }
    }

    return items;
}

std::vector<std::shared_ptr<Object>> Equipment::get_equipped_by_type(ObjectType type) const {
    std::vector<std::shared_ptr<Object>> items;

    for (const auto& [slot, item] : equipped_) {
        if (item && item->type() == type) {
            items.push_back(item);
        }
    }

    return items;
}

int Equipment::calculate_total_armor_class() const {
    int total_ac = 0;

    for (const auto& [slot, item] : equipped_) {
        if (item) {
            total_ac += item->armor_class();
        }
    }

    return total_ac;
}

std::shared_ptr<Object> Equipment::get_main_weapon() const {
    return get_equipped(EquipSlot::Wield);
}

std::shared_ptr<Object> Equipment::get_off_weapon() const {
    return get_equipped(EquipSlot::Wield2);
}

bool Equipment::is_wielding_two_handed() const {
    auto weapon = get_main_weapon();
    if (!weapon) return false;

    // True two-handed weapon
    if (weapon->has_flag(ObjectFlag::TwoHanded)) return true;

    // Versatile weapon being used with two-handed grip
    if (weapon->has_flag(ObjectFlag::Versatile) && using_two_handed_grip_) return true;

    return false;
}

std::string Equipment::toggle_grip() {
    auto weapon = get_main_weapon();
    if (!weapon) {
        return "You aren't wielding anything.";
    }

    if (!weapon->has_flag(ObjectFlag::Versatile)) {
        return "Your weapon cannot be wielded with a different grip.";
    }

    if (using_two_handed_grip_) {
        // Switching from 2H to 1H
        using_two_handed_grip_ = false;
        return "";  // Success
    } else {
        // Switching from 1H to 2H - check if offhand is occupied
        auto offhand = get_equipped(EquipSlot::Hold);
        auto shield = get_equipped(EquipSlot::Shield);

        if (offhand || shield) {
            return "You need both hands free to use a two-handed grip.";
        }

        using_two_handed_grip_ = true;
        return "";  // Success
    }
}

std::vector<std::shared_ptr<Object>> Equipment::clear_all() {
    auto items = get_all_equipped();
    equipped_.clear();
    return items;
}

bool Equipment::has_slot_conflict(EquipSlot slot, const Object& item) const {
    // First check: Is this slot already occupied?
    if (is_equipped(slot)) {
        return true;
    }

    // Check for two-handed weapon conflicts
    if (item.has_flag(ObjectFlag::TwoHanded) && slot == EquipSlot::Wield) {
        return is_equipped(EquipSlot::Wield2) || is_equipped(EquipSlot::Shield);
    }

    if (slot == EquipSlot::Wield2 || slot == EquipSlot::Shield) {
        auto main_weapon = get_equipped(EquipSlot::Wield);
        if (main_weapon && main_weapon->has_flag(ObjectFlag::TwoHanded)) {
            return true;
        }
    }

    return false;
}

nlohmann::json Equipment::to_json() const {
    nlohmann::json json;

    for (const auto& [slot, item] : equipped_) {
        if (item) {
            std::string slot_name = std::string(magic_enum::enum_name(slot));
            json[slot_name] = item->to_json();
        }
    }

    return json;
}

Result<Equipment> Equipment::from_json(const nlohmann::json& json) {
    try {
        Equipment equipment;

        for (const auto& [slot_name, item_json] : json.items()) {
            if (auto slot = magic_enum::enum_cast<EquipSlot>(slot_name)) {
                auto item_result = Object::from_json(item_json);
                if (item_result) {
                    equipment.equipped_[slot.value()] = std::move(item_result.value());
                } else {
                    Log::error("Failed to load equipped item in slot {}: {}",
                              slot_name, item_result.error().message);
                }
            }
        }

        return equipment;

    } catch (const nlohmann::json::exception& e) {
        return std::unexpected(Errors::ParseError("Equipment JSON parsing error", e.what()));
    }
}

// Actor Implementation

Actor::Actor(EntityId id, std::string_view name)
    : Entity(id, name), spell_slots_(std::make_unique<SpellSlots>()) {}

Actor::Actor(EntityId id, std::string_view name,
             const Stats& stats, const Inventory& inventory, const Equipment& equipment)
    : Entity(id, name), stats_(stats), inventory_(inventory), equipment_(equipment),
      spell_slots_(std::make_unique<SpellSlots>()) {}

bool Actor::has_flag(ActorFlag flag) const {
    return flags_.contains(flag);
}

void Actor::set_flag(ActorFlag flag, bool value) {
    if (value) {
        flags_.insert(flag);
    } else {
        flags_.erase(flag);
    }
}

std::shared_ptr<Room> Actor::current_room() const {
    return current_room_.lock();
}

Result<void> Actor::move_to(std::shared_ptr<Room> new_room) {
    auto old_room = current_room_.lock();

    if (old_room == new_room) {
        return Success(); // Already in the target room
    }

    current_room_ = new_room;
    on_room_change(old_room, new_room);

    return Success();
}

bool Actor::is_visible_to(const Actor& observer) const {
    // HolyLight bypasses all visibility checks
    if (observer.is_holylight()) {
        return true;
    }

    if (has_flag(ActorFlag::Invisible) && !observer.has_flag(ActorFlag::Detect_Invis)) {
        return false;
    }

    if (has_flag(ActorFlag::Hide) && !observer.has_flag(ActorFlag::Sense_Life)) {
        return false;
    }

    if (observer.has_flag(ActorFlag::Blind)) {
        return false;
    }

    return true;
}

bool Actor::can_see(const Actor& target) const {
    if (has_flag(ActorFlag::Blind)) {
        return false;
    }

    return target.is_visible_to(*this);
}

bool Actor::can_see(const Object& object) const {
    // HolyLight bypasses all visibility checks
    if (is_holylight()) {
        return true;
    }

    if (has_flag(ActorFlag::Blind)) {
        return false;
    }

    if (object.has_flag(ObjectFlag::Invisible) && !has_flag(ActorFlag::Detect_Invis)) {
        return false;
    }

    return true;
}

std::string Actor::display_name_for_observer(const Actor& observer) const {
    std::string result = display_name();
    std::string indicators;

    // Check if observer can see invisible status (via Detect_Invis)
    if (has_flag(ActorFlag::Invisible) && observer.has_flag(ActorFlag::Detect_Invis)) {
        indicators += "(invis)";
    }

    // Check if observer can see alignment (via Detect_Align)
    if (observer.has_flag(ActorFlag::Detect_Align)) {
        int alignment = stats().alignment;
        if (alignment <= -350) {
            indicators += indicators.empty() ? "(evil)" : " (evil)";
        } else if (alignment >= 350) {
            indicators += indicators.empty() ? "(good)" : " (good)";
        }
    }

    // Check for magical aura (via Detect_Magic)
    if (observer.has_flag(ActorFlag::Detect_Magic)) {
        if (!active_effects_.empty()) {
            indicators += indicators.empty() ? "(magical)" : " (magical)";
        }
    }

    // Check for hidden status (via Sense_Life)
    if (has_flag(ActorFlag::Hide) && observer.has_flag(ActorFlag::Sense_Life)) {
        indicators += indicators.empty() ? "(hidden)" : " (hidden)";
    }

    if (!indicators.empty()) {
        result = indicators + " " + result;
    }

    return result;
}

std::string Actor::room_presence(std::shared_ptr<Actor> viewer) const {
    // For NPCs with a ground/long description, use that if they're in default position
    // and not in a special state
    auto ground_desc = ground();
    bool use_ground_desc = !ground_desc.empty() &&
                           position() == Position::Standing &&
                           !is_fighting() &&
                           !has_flag(ActorFlag::Paralyzed) &&
                           !has_flag(ActorFlag::Immobilized);

    if (use_ground_desc) {
        return std::string(ground_desc);
    }

    // Otherwise, generate a position-based description
    // For players, use display_name(); for NPCs, use short_description or display_name
    std::string name;
    auto short_desc = short_description();
    if (short_desc.empty()) {
        name = display_name();
    } else {
        name = std::string(short_desc);
    }

    // Check for special states first
    if (has_flag(ActorFlag::Paralyzed)) {
        return fmt::format("{} is standing here, completely motionless.", name);
    }

    if (has_flag(ActorFlag::Immobilized)) {
        return fmt::format("{} is here, unable to move.", name);
    }

    // Fighting takes precedence
    if (is_fighting()) {
        auto opponent = get_fighting_target();
        if (opponent) {
            if (viewer && opponent == viewer) {
                return fmt::format("{} is here, fighting <red>YOU</>!", name);
            } else {
                return fmt::format("{} is here, fighting {}!", name, opponent->display_name());
            }
        } else {
            return fmt::format("{} is here, struggling with thin air.", name);
        }
    }

    // Position-based descriptions
    switch (position()) {
    case Position::Dead:
        return fmt::format("{} is lying here, dead.", name);
    case Position::Ghost:
        return fmt::format("The ghost of {} hovers here.", name);
    case Position::Mortally_Wounded:
        return fmt::format("{} is lying here, mortally wounded.", name);
    case Position::Incapacitated:
        return fmt::format("{} is lying here, incapacitated.", name);
    case Position::Stunned:
        return fmt::format("{} is lying here, stunned.", name);
    case Position::Sleeping:
        return fmt::format("{} is sleeping here.", name);
    case Position::Resting:
        return fmt::format("{} is resting here.", name);
    case Position::Sitting:
        return fmt::format("{} is sitting here.", name);
    case Position::Prone:
        return fmt::format("{} is lying here.", name);
    case Position::Fighting:
        // Already handled above, but just in case
        return fmt::format("{} is here, fighting.", name);
    case Position::Standing:
        return fmt::format("{} is standing here.", name);
    case Position::Flying:
        return fmt::format("{} is hovering here.", name);
    default:
        return fmt::format("{} is here.", name);
    }
}

Result<void> Actor::give_item(std::shared_ptr<Object> item) {
    if (!item) {
        return std::unexpected(Errors::InvalidArgument("item", "cannot be null"));
    }

    if (!inventory_.can_carry(item->weight(), max_carry_weight())) {
        return std::unexpected(Errors::InvalidState(fmt::format(
            "Cannot carry that much weight: item {} weighs {}, current weight {}/{}",
            item->name(), item->weight(), current_carry_weight(), max_carry_weight())));
    }

    return inventory_.add_item(std::move(item));
}

std::shared_ptr<Object> Actor::take_item(EntityId item_id) {
    return inventory_.remove_item(item_id);
}

Result<void> Actor::equip(EntityId item_id) {
    auto item = inventory_.remove_item(item_id);
    if (!item) {
        return std::unexpected(Errors::NotFound("Item not found in inventory"));
    }

    auto result = equipment_.equip_item(item);
    if (!result) {
        // Put item back in inventory if equip failed
        TRY(inventory_.add_item(item));
        return std::unexpected(result.error());
    }

    return Success();
}

std::shared_ptr<Object> Actor::unequip(EquipSlot slot) {
    auto item = equipment_.unequip_item(slot);
    if (item) {
        // Try to put in inventory, if that fails, drop it
        if (auto result = inventory_.add_item(item); !result) {
            Log::warn("Could not return unequipped item to inventory: {}", result.error().message);
            // In a real implementation, you'd drop it in the room
        }
    }
    return item;
}

int Actor::max_carry_weight() const {
    // Base capacity increased for better gameplay balance
    // Typical objects weigh 1-20 pounds, so we need reasonable capacity
    return BASE_CARRY_CAPACITY + Stats::attribute_modifier(stats_.strength) * CARRY_CAPACITY_PER_STR_MOD;
}

int Actor::current_carry_weight() const {
    int total = inventory_.total_weight();

    for (const auto& item : equipment_.get_all_equipped()) {
        if (item) {
            total += item->weight();
        }
    }

    return total;
}

bool Actor::is_overloaded() const {
    return current_carry_weight() > max_carry_weight();
}

void Actor::gain_experience(long amount) {
    if (amount <= 0) return;

    int old_level = stats_.level;
    stats_.experience += amount;

    while (stats_.can_level_up() && stats_.level < MAX_LEVEL) {
        auto result = level_up();
        if (!result) {
            Log::error("Failed to level up {}: {}", name(), result.error().message);
            break;
        }
    }

    if (stats_.level > old_level) {
        on_level_up(old_level, stats_.level);
    }
}

Result<void> Actor::level_up() {
    if (!stats_.can_level_up()) {
        return std::unexpected(Errors::InvalidState("Not enough experience to level up"));
    }

    stats_.level++;

    // Increase hit points
    int hp_gain = 8 + Stats::attribute_modifier(stats_.constitution);
    stats_.max_hit_points += std::max(1, hp_gain);
    stats_.hit_points = stats_.max_hit_points; // Full heal on level up

    // Increase mana
    int mana_gain = 4 + Stats::attribute_modifier(stats_.intelligence);
    stats_.max_mana += std::max(1, mana_gain);
    stats_.mana = stats_.max_mana;

    // Increase stamina
    int stamina_gain = 2 + Stats::attribute_modifier(stats_.constitution) / 2;
    stats_.max_stamina += std::max(1, stamina_gain);
    stats_.stamina = stats_.max_stamina;

    return Success();
}

nlohmann::json Actor::to_json() const {
    nlohmann::json json = Entity::to_json();

    json["stats"] = stats_.to_json();
    json["inventory"] = inventory_.to_json();
    json["equipment"] = equipment_.to_json();
    json["position"] = std::string(magic_enum::enum_name(position_));

    std::vector<std::string> flag_names;
    for (ActorFlag flag : flags_) {
        flag_names.emplace_back(magic_enum::enum_name(flag));
    }
    json["flags"] = flag_names;

    // Add spell slots if they exist
    if (spell_slots_) {
        json["spell_slots"] = spell_slots_->to_json();
    }

    // Serialize active effects
    nlohmann::json effects_json = nlohmann::json::array();
    for (const auto& effect : active_effects_) {
        nlohmann::json eff_json;
        eff_json["effect_id"] = effect.effect_id;
        eff_json["name"] = effect.name;
        eff_json["source"] = effect.source;
        eff_json["flag"] = std::string(magic_enum::enum_name(effect.flag));
        eff_json["duration_hours"] = effect.duration_hours;
        eff_json["modifier_value"] = effect.modifier_value;
        eff_json["modifier_stat"] = effect.modifier_stat;
        effects_json.push_back(eff_json);
    }
    json["active_effects"] = effects_json;

    // Serialize DoT effects
    nlohmann::json dots_json = nlohmann::json::array();
    for (const auto& dot : dot_effects_) {
        nlohmann::json dot_json;
        dot_json["ability_id"] = dot.ability_id;
        dot_json["effect_id"] = dot.effect_id;
        dot_json["effect_type"] = dot.effect_type;
        dot_json["damage_type"] = dot.damage_type;
        dot_json["cure_category"] = dot.cure_category;
        dot_json["potency"] = dot.potency;
        dot_json["flat_damage"] = dot.flat_damage;
        dot_json["percent_damage"] = dot.percent_damage;
        dot_json["blocks_regen"] = dot.blocks_regen;
        dot_json["reduces_regen"] = dot.reduces_regen;
        dot_json["remaining_ticks"] = dot.remaining_ticks;
        dot_json["tick_interval"] = dot.tick_interval;
        dot_json["ticks_since_last"] = dot.ticks_since_last;
        dot_json["source_actor_id"] = dot.source_actor_id;
        dot_json["source_level"] = dot.source_level;
        dot_json["stack_count"] = dot.stack_count;
        dot_json["max_stacks"] = dot.max_stacks;
        dot_json["stackable"] = dot.stackable;
        dots_json.push_back(dot_json);
    }
    json["dot_effects"] = dots_json;

    // Serialize HoT effects
    nlohmann::json hots_json = nlohmann::json::array();
    for (const auto& hot : hot_effects_) {
        nlohmann::json hot_json;
        hot_json["ability_id"] = hot.ability_id;
        hot_json["effect_id"] = hot.effect_id;
        hot_json["effect_type"] = hot.effect_type;
        hot_json["heal_type"] = hot.heal_type;
        hot_json["hot_category"] = hot.hot_category;
        hot_json["flat_heal"] = hot.flat_heal;
        hot_json["percent_heal"] = hot.percent_heal;
        hot_json["boosts_regen"] = hot.boosts_regen;
        hot_json["regen_boost"] = hot.regen_boost;
        hot_json["remaining_ticks"] = hot.remaining_ticks;
        hot_json["tick_interval"] = hot.tick_interval;
        hot_json["ticks_since_last"] = hot.ticks_since_last;
        hot_json["source_actor_id"] = hot.source_actor_id;
        hot_json["source_level"] = hot.source_level;
        hot_json["stack_count"] = hot.stack_count;
        hot_json["max_stacks"] = hot.max_stacks;
        hot_json["stackable"] = hot.stackable;
        hots_json.push_back(hot_json);
    }
    json["hot_effects"] = hots_json;

    return json;
}

Result<void> Actor::validate() const {
    TRY(Entity::validate());
    TRY(stats_.validate());

    return Success();
}

// Spell system methods
bool Actor::can_cast_spell(std::string_view spell_name) const {
    auto* spell = SpellRegistry::instance().find_spell(spell_name);
    if (!spell) {
        return false;
    }

    return spell->can_cast(*this) && has_spell_slots(spell->circle);
}

bool Actor::has_spell_slots(int circle) const {
    return spell_slots_ && spell_slots_->has_slots(circle);
}

bool Actor::consume_spell_slot(int circle) {
    return spell_slots_ && spell_slots_->consume_slot(circle);
}

std::pair<int, int> Actor::get_spell_slot_info(int circle) const {
    if (!spell_slots_) {
        return {0, 0};
    }

    return spell_slots_->get_slot_info(circle);
}

const SpellSlots& Actor::spell_slots() const {
    return *spell_slots_;
}

SpellSlots& Actor::spell_slots() {
    return *spell_slots_;
}

// Active Effect Management

void Actor::add_effect(const ActiveEffect& effect) {
    // Remove any existing effect with the same name first (refresh/overwrite)
    remove_effect(effect.name);

    // Add the new effect
    active_effects_.push_back(effect);

    // Apply the associated flag if set
    if (effect.flag != ActorFlag::None) {
        set_flag(effect.flag, true);
    }

    Log::game()->debug("Effect '{}' applied to {} (duration: {} hours)",
                      effect.name, name(), effect.duration_hours);
}

void Actor::remove_effect(const std::string& effect_name) {
    auto it = std::find_if(active_effects_.begin(), active_effects_.end(),
                          [&effect_name](const ActiveEffect& e) {
                              return e.name == effect_name;
                          });

    if (it != active_effects_.end()) {
        // Remove the associated flag
        if (it->flag != ActorFlag::None) {
            set_flag(it->flag, false);
        }

        Log::game()->debug("Effect '{}' removed from {}", effect_name, name());
        active_effects_.erase(it);
    }
}

bool Actor::has_effect(const std::string& effect_name) const {
    return std::any_of(active_effects_.begin(), active_effects_.end(),
                      [&effect_name](const ActiveEffect& e) {
                          return e.name == effect_name;
                      });
}

const ActiveEffect* Actor::get_effect(const std::string& effect_name) const {
    auto it = std::find_if(active_effects_.begin(), active_effects_.end(),
                          [&effect_name](const ActiveEffect& e) {
                              return e.name == effect_name;
                          });
    return it != active_effects_.end() ? &(*it) : nullptr;
}

void Actor::tick_effects() {
    // Decrement durations and remove expired effects (called each MUD hour)
    for (auto it = active_effects_.begin(); it != active_effects_.end(); ) {
        if (!it->is_permanent()) {
            it->duration_hours--;
            if (it->duration_hours <= 0) {
                // Notify the actor that the effect has worn off
                send_message(fmt::format("Your {} effect wears off.\r\n", it->name));

                // Remove the associated flag
                if (it->flag != ActorFlag::None) {
                    set_flag(it->flag, false);
                }
                Log::game()->debug("Effect '{}' expired on {}", it->name, name());
                it = active_effects_.erase(it);
                continue;
            }
        }
        ++it;
    }
}

void Actor::clear_effects() {
    // Remove all flags
    for (const auto& effect : active_effects_) {
        if (effect.flag != ActorFlag::None) {
            set_flag(effect.flag, false);
        }
    }
    active_effects_.clear();
}

// =============================================================================
// Periodic Character Tick System
// =============================================================================

double Actor::get_regen_multiplier() const {
    // Position-based regeneration multipliers (from legacy limits.cpp)
    // Sleeping = 5x, Resting = 3x, Sitting = 1.5x, Fighting = 0.5x, Standing = 1x
    switch (position_) {
        case Position::Sleeping:
            return 5.0;
        case Position::Resting:
            return 3.0;
        case Position::Sitting:
            return 1.5;
        case Position::Fighting:
            return 0.5;
        case Position::Dead:
        case Position::Ghost:
        case Position::Mortally_Wounded:
        case Position::Incapacitated:
        case Position::Stunned:
            return 0.0;  // No regen while dying/stunned
        default:
            return 1.0;  // Standing, Flying, Prone
    }
}

int Actor::calculate_hit_gain() const {
    // Check if any DoT effect blocks regeneration completely
    if (is_regen_blocked_by_dot()) {
        return 0;
    }

    // Base regeneration per 4-second tick (legacy-style fast ticks)
    // With ~19 ticks per MUD hour, we scale appropriately
    // Standing: slow regen, Resting: moderate, Sleeping: fast
    int max_hp = stats_.max_hit_points;

    // Base is 1 HP per 2 ticks while standing for typical character
    // This gives ~30 HP/minute standing, ~90 HP/minute sleeping
    int gain = std::max(1, max_hp / 100);  // Base: 1% per tick

    // Add constitution modifier bonus
    int con_mod = Stats::attribute_modifier(stats_.constitution);
    if (con_mod > 0) {
        gain += con_mod;
    }

    // Apply position multiplier
    double multiplier = get_regen_multiplier();
    gain = static_cast<int>(gain * multiplier);

    // Song of Rest effect bonus (triple gain when resting/sleeping)
    if (has_effect("Song of Rest") &&
        (position_ == Position::Sleeping || position_ == Position::Resting)) {
        gain *= 3;
    }

    // Race bonus: Trolls regenerate at 2x rate
    if (race_ == "Troll") {
        gain *= 2;
    }

    // Apply regen reduction from DoT effects (stacks additively)
    int regen_reduction = get_dot_regen_reduction();
    if (regen_reduction > 0) {
        gain = gain * (100 - regen_reduction) / 100;
    }

    // Apply regen boost from HoT effects (stacks additively)
    int regen_boost = get_hot_regen_boost();
    if (regen_boost > 0) {
        gain = gain * (100 + regen_boost) / 100;
    }

    // Nourished effect bonus (from eating food)
    if (has_effect("Nourished")) {
        gain = gain * 3 / 2;  // 50% bonus HP regen when well-fed
    }

    return std::max(0, gain);
}

int Actor::calculate_stamina_gain() const {
    // Check if any DoT effect blocks regeneration completely
    if (is_regen_blocked_by_dot()) {
        return 0;
    }

    // Base regeneration per 4-second tick (legacy-style fast ticks)
    // Stamina recovers faster than HP - about 2% per tick base
    int max_stamina = stats_.max_stamina;
    int gain = std::max(1, max_stamina / 50);  // Base: 2% per tick

    // Apply position multiplier
    double multiplier = get_regen_multiplier();
    gain = static_cast<int>(gain * multiplier);

    // Song of Rest effect bonus
    if (has_effect("Song of Rest") &&
        (position_ == Position::Sleeping || position_ == Position::Resting)) {
        gain = static_cast<int>(gain * 1.5);
    }

    // Apply regen reduction from DoT effects (stacks additively)
    int regen_reduction = get_dot_regen_reduction();
    if (regen_reduction > 0) {
        gain = gain * (100 - regen_reduction) / 100;
    }

    // Refreshed effect bonus (from drinking)
    if (has_effect("Refreshed")) {
        gain = gain * 3 / 2;  // 50% bonus stamina regen when refreshed
    }

    return std::max(0, gain);
}

Actor::TickResult Actor::perform_regen_tick() {
    TickResult result;

    // Handle dying characters (stunned/incapacitated/mortally wounded)
    if (position_ == Position::Stunned ||
        position_ == Position::Incapacitated ||
        position_ == Position::Mortally_Wounded) {
        // Take 1 HP damage per tick while dying
        result.dying_damage = 1;
        stats_.hit_points -= 1;

        if (stats_.hit_points <= 0) {
            stats_.hit_points = 0;
            result.died = true;
            send_message("You have died!\r\n");
        } else {
            send_message("You feel your life slipping away...\r\n");
        }
        return result;  // No other processing while dying
    }

    // Skip processing for dead/ghost
    if (position_ == Position::Dead || position_ == Position::Ghost) {
        return result;
    }

    // Process all DoT effects first (before regen) - data-driven system
    if (!dot_effects_.empty()) {
        auto dot_result = process_dot_effects();
        result.poison_damage = dot_result.total_damage;

        // Copy expired effect names
        for (const auto& expired : dot_result.expired_effects) {
            result.expired_effects.push_back(expired);
        }

        if (dot_result.died) {
            result.died = true;
            return result;
        }
    }

    // Process all HoT effects - data-driven system
    if (!hot_effects_.empty()) {
        auto hot_result = process_hot_effects();
        result.hot_healing = hot_result.total_healing;

        // Copy expired effect names
        for (const auto& expired : hot_result.expired_effects) {
            result.expired_effects.push_back(expired);
        }
    }

    // HP Regeneration
    if (stats_.hit_points < stats_.max_hit_points) {
        int hp_gain = calculate_hit_gain();
        if (hp_gain > 0) {
            int old_hp = stats_.hit_points;
            stats_.hit_points = std::min(stats_.hit_points + hp_gain, stats_.max_hit_points);
            result.hp_gained = stats_.hit_points - old_hp;
        }
    }

    // Stamina Regeneration
    if (stats_.stamina < stats_.max_stamina) {
        int stamina_gain = calculate_stamina_gain();
        if (stamina_gain > 0) {
            int old_stamina = stats_.stamina;
            stats_.stamina = std::min(stats_.stamina + stamina_gain, stats_.max_stamina);
            result.stamina_gained = stats_.stamina - old_stamina;

            // Notify player of significant stamina recovery (only for players)
            if (result.stamina_gained > 0 && dynamic_cast<Player*>(this) != nullptr) {
                if (stats_.stamina == stats_.max_stamina) {
                    send_message("You feel fully rested.\r\n");
                }
            }
        }
    }

    return result;
}

Actor::TickResult Actor::perform_hour_tick() {
    TickResult result;

    // Skip processing for dead/ghost/dying
    if (position_ == Position::Dead || position_ == Position::Ghost ||
        position_ == Position::Stunned || position_ == Position::Incapacitated ||
        position_ == Position::Mortally_Wounded) {
        return result;
    }

    // Tick effect durations (decrements by 1 hour)
    for (auto it = active_effects_.begin(); it != active_effects_.end(); ) {
        if (!it->is_permanent()) {
            it->duration_hours--;
            if (it->duration_hours <= 0) {
                result.expired_effects.push_back(it->name);
                send_message(fmt::format("Your {} effect wears off.\r\n", it->name));

                // Remove the associated flag
                if (it->flag != ActorFlag::None) {
                    set_flag(it->flag, false);
                }
                Log::game()->debug("Effect '{}' expired on {}", it->name, name());
                it = active_effects_.erase(it);
                continue;
            }
        }
        ++it;
    }

    // Process drunk depletion (sobering up) - only for players
    // Note: Hunger/thirst removed - eating/drinking now provides buffs instead
    if (dynamic_cast<Player*>(this) != nullptr) {
        int old_drunk = stats_.drunk;
        if (stats_.drunk > Stats::CONDITION_MIN) {
            stats_.drunk = std::max(Stats::CONDITION_MIN, stats_.drunk - 1);

            // Send messages when sobering up
            if (old_drunk > Stats::DRUNK_TOO_DRUNK_THRESHOLD &&
                stats_.drunk <= Stats::DRUNK_TOO_DRUNK_THRESHOLD) {
                send_message("You feel less intoxicated.\r\n");
            }
            if (old_drunk > Stats::DRUNK_SLURRED_THRESHOLD &&
                stats_.drunk <= Stats::DRUNK_SLURRED_THRESHOLD) {
                send_message("Your head starts to clear.\r\n");
            }
            if (old_drunk > Stats::CONDITION_MIN && stats_.drunk == Stats::CONDITION_MIN) {
                send_message("You are now sober.\r\n");
            }
        }
    }

    return result;
}

bool Actor::interrupt_concentration() {
    bool interrupted = false;

    // Interrupt player meditation (new flag-based system)
    if (auto* player = dynamic_cast<Player*>(this)) {
        if (player->is_meditating()) {
            player->stop_meditation();
            send_message("Your meditation is broken.\n");
            interrupted = true;
        }
    }

    // Interrupt legacy concentration effects
    static const std::vector<std::string> concentration_effects = {
        "Meditate"
    };

    for (const auto& effect_name : concentration_effects) {
        if (has_effect(effect_name)) {
            remove_effect(effect_name);
            send_message("Your concentration is broken.\n");
            interrupted = true;
        }
    }

    return interrupted;
}

// Casting blackout system implementation

bool Actor::is_casting_blocked() const {
    auto now = std::chrono::steady_clock::now();
    return casting_blackout_end_ > now;
}

std::chrono::milliseconds Actor::get_blackout_remaining() const {
    auto now = std::chrono::steady_clock::now();
    if (casting_blackout_end_ <= now) {
        return std::chrono::milliseconds{0};
    }
    return std::chrono::duration_cast<std::chrono::milliseconds>(casting_blackout_end_ - now);
}

void Actor::set_casting_blackout(std::chrono::milliseconds duration) {
    casting_blackout_end_ = std::chrono::steady_clock::now() + duration;
}

void Actor::clear_casting_blackout() {
    casting_blackout_end_ = std::chrono::steady_clock::time_point{};
}

void Actor::queue_spell(std::string_view spell_name, std::string_view target_name) {
    queued_spell_ = QueuedSpell{
        .spell_name = std::string{spell_name},
        .target_name = std::string{target_name},
        .queued_at = std::chrono::steady_clock::now()
    };
}

std::optional<Actor::QueuedSpell> Actor::pop_queued_spell() {
    if (!queued_spell_.has_value()) {
        return std::nullopt;
    }
    auto spell = std::move(queued_spell_);
    queued_spell_.reset();
    return spell;
}

// ============================================================================
// Casting Time System Implementation
// ============================================================================

void Actor::start_casting(const CastingState& state) {
    casting_state_ = state;
}

void Actor::stop_casting(bool interrupted) {
    if (!casting_state_.has_value()) {
        return;
    }

    if (interrupted) {
        Log::game()->debug("{}'s casting of {} was interrupted",
                           name(), casting_state_->ability_name);
        send_message("Your concentration is broken!\n");
    }

    casting_state_.reset();
}

bool Actor::advance_casting() {
    if (!casting_state_.has_value()) {
        return false;
    }

    // Decrement casting time by 1 tick per 500ms interval
    // With CAST_SPEED_PER_ROUND = 8 representing one 4-second combat round,
    // this means 8 ticks = 4 seconds = 1 combat round
    casting_state_->ticks_remaining -= 1;

    if (casting_state_->ticks_remaining <= 0) {
        // Casting complete
        Log::game()->debug("{} completes casting {}", name(), casting_state_->ability_name);
        return true;
    }

    // Progress message is sent by WorldServer::perform_casting_processing()
    return false;
}

int Actor::get_casting_progress_percent() const {
    if (!casting_state_.has_value() || casting_state_->total_ticks == 0) {
        return 100;
    }
    int elapsed = casting_state_->total_ticks - casting_state_->ticks_remaining;
    return (elapsed * 100) / casting_state_->total_ticks;
}

// ============================================================================
// Fighting List Management
// ============================================================================

void Actor::add_enemy(std::shared_ptr<Actor> enemy) {
    if (!enemy || enemy.get() == this) return;

    // Clean up expired weak pointers
    fighting_list_.erase(
        std::remove_if(fighting_list_.begin(), fighting_list_.end(),
            [](const std::weak_ptr<Actor>& wp) { return wp.expired(); }),
        fighting_list_.end());

    // Check if already in list
    for (const auto& wp : fighting_list_) {
        if (auto existing = wp.lock()) {
            if (existing == enemy) return;  // Already fighting this enemy
        }
    }

    fighting_list_.push_back(enemy);
    Log::game()->debug("{} added {} to fighting list", name(), enemy->name());
}

void Actor::remove_enemy(std::shared_ptr<Actor> enemy) {
    if (!enemy) return;

    fighting_list_.erase(
        std::remove_if(fighting_list_.begin(), fighting_list_.end(),
            [&enemy](const std::weak_ptr<Actor>& wp) {
                auto locked = wp.lock();
                return !locked || locked == enemy;
            }),
        fighting_list_.end());
}

void Actor::clear_enemies() {
    fighting_list_.clear();
}

bool Actor::has_enemies() const {
    for (const auto& wp : fighting_list_) {
        if (auto enemy = wp.lock()) {
            if (enemy->is_alive()) return true;
        }
    }
    return false;
}

std::shared_ptr<Actor> Actor::get_fighting_target() const {
    // Return first valid, alive enemy in the same room
    auto my_room = current_room();
    for (const auto& wp : fighting_list_) {
        if (auto enemy = wp.lock()) {
            if (enemy->is_alive() && enemy->current_room() == my_room) {
                return enemy;
            }
        }
    }
    return nullptr;
}

std::vector<std::shared_ptr<Actor>> Actor::get_all_enemies() const {
    std::vector<std::shared_ptr<Actor>> enemies;
    for (const auto& wp : fighting_list_) {
        if (auto enemy = wp.lock()) {
            if (enemy->is_alive()) {
                enemies.push_back(enemy);
            }
        }
    }
    return enemies;
}

void Actor::set_primary_target(std::shared_ptr<Actor> enemy) {
    if (!enemy) return;

    // Remove from current position
    fighting_list_.erase(
        std::remove_if(fighting_list_.begin(), fighting_list_.end(),
            [&enemy](const std::weak_ptr<Actor>& wp) {
                auto locked = wp.lock();
                return !locked || locked == enemy;
            }),
        fighting_list_.end());

    // Insert at front
    fighting_list_.insert(fighting_list_.begin(), enemy);
}

// ============================================================================
// Data-Driven DoT Effect System Implementation
// ============================================================================

void Actor::add_dot_effect(const fiery::DotEffect& effect) {
    // Check for stacking
    if (effect.stackable) {
        // Find existing effect of same type
        for (auto& existing : dot_effects_) {
            if (existing.ability_id == effect.ability_id &&
                existing.effect_id == effect.effect_id) {
                if (existing.stack_count < existing.max_stacks) {
                    existing.stack_count++;
                    // Refresh duration
                    existing.remaining_ticks = effect.remaining_ticks;
                    Log::game()->debug("DoT effect {} stacked to {} on {}",
                                       effect.damage_type, existing.stack_count, name());
                }
                return;
            }
        }
    }

    // Add new effect
    dot_effects_.push_back(effect);
    update_dot_flags();
    Log::game()->debug("DoT effect {} (potency {}) applied to {}",
                       effect.damage_type, effect.potency, name());
}

void Actor::remove_dot_effects_by_category(const std::string& cure_category) {
    auto it = std::remove_if(dot_effects_.begin(), dot_effects_.end(),
        [&cure_category](const fiery::DotEffect& e) {
            return e.matches_cure_category(cure_category);
        });
    dot_effects_.erase(it, dot_effects_.end());
    update_dot_flags();
}

bool Actor::has_dot_effect(const std::string& cure_category) const {
    return std::any_of(dot_effects_.begin(), dot_effects_.end(),
        [&cure_category](const fiery::DotEffect& e) {
            return e.matches_cure_category(cure_category);
        });
}

bool Actor::is_regen_blocked_by_dot() const {
    return std::any_of(dot_effects_.begin(), dot_effects_.end(),
        [](const fiery::DotEffect& e) {
            return e.blocks_regen;
        });
}

int Actor::get_dot_regen_reduction() const {
    int total_reduction = 0;
    for (const auto& effect : dot_effects_) {
        total_reduction += effect.reduces_regen;
    }
    return std::min(100, total_reduction);  // Cap at 100%
}

fiery::CureAttemptResult Actor::attempt_cure(const std::string& cure_category,
                                              int cure_power,
                                              bool cure_all,
                                              bool partial_on_fail) {
    fiery::CureAttemptResult result;
    result.result = fiery::CureResult::NoEffect;

    // Find effects matching the cure category, sorted by potency (highest first)
    std::vector<size_t> matching_indices;
    for (size_t i = 0; i < dot_effects_.size(); ++i) {
        if (dot_effects_[i].matches_cure_category(cure_category)) {
            matching_indices.push_back(i);
        }
    }

    if (matching_indices.empty()) {
        result.result = fiery::CureResult::WrongCategory;
        result.message = fmt::format("You have no {} effects to cure.", cure_category);
        return result;
    }

    // Sort by potency descending
    std::sort(matching_indices.begin(), matching_indices.end(),
        [this](size_t a, size_t b) {
            return dot_effects_[a].potency > dot_effects_[b].potency;
        });

    std::vector<size_t> to_remove;

    for (size_t idx : matching_indices) {
        auto& effect = dot_effects_[idx];

        if (cure_power >= effect.potency) {
            // Full cure - mark for removal
            to_remove.push_back(idx);
            result.effects_removed++;
            result.result = fiery::CureResult::FullCure;
        } else if (partial_on_fail && cure_power >= effect.potency - 2) {
            // Partial cure - reduce duration by 50% and potency by 1
            effect.remaining_ticks = std::max(1, effect.remaining_ticks / 2);
            effect.potency = std::max(1, effect.potency - 1);
            result.effects_reduced++;
            if (result.result != fiery::CureResult::FullCure) {
                result.result = fiery::CureResult::PartialCure;
            }
        } else {
            // Cure too weak
            if (result.result == fiery::CureResult::NoEffect) {
                result.result = fiery::CureResult::TooPowerful;
                result.message = fmt::format("The {} is too powerful for your cure!",
                                             effect.damage_type);
            }
        }

        // If not curing all, stop after first attempt
        if (!cure_all) break;
    }

    // Remove fully cured effects (reverse order to preserve indices)
    std::sort(to_remove.begin(), to_remove.end(), std::greater<size_t>());
    for (size_t idx : to_remove) {
        dot_effects_.erase(dot_effects_.begin() + static_cast<ptrdiff_t>(idx));
    }

    // Update flags
    update_dot_flags();

    // Build result message
    if (result.effects_removed > 0 && result.effects_reduced > 0) {
        result.message = fmt::format("You cure {} effect{} and weaken {} more.",
                                     result.effects_removed,
                                     result.effects_removed == 1 ? "" : "s",
                                     result.effects_reduced);
    } else if (result.effects_removed > 0) {
        result.message = fmt::format("You cure {} {} effect{}.",
                                     result.effects_removed,
                                     cure_category,
                                     result.effects_removed == 1 ? "" : "s");
    } else if (result.effects_reduced > 0) {
        result.message = fmt::format("You weaken {} {} effect{}, but cannot fully cure them.",
                                     result.effects_reduced,
                                     cure_category,
                                     result.effects_reduced == 1 ? "" : "s");
    }

    return result;
}

Actor::DotTickResult Actor::process_dot_effects() {
    DotTickResult result;
    std::vector<size_t> expired_indices;

    for (size_t i = 0; i < dot_effects_.size(); ++i) {
        auto& effect = dot_effects_[i];

        // Advance tick counter
        effect.advance_tick();

        // Check if it's time to apply damage
        if (!effect.should_tick()) continue;
        effect.reset_tick_counter();

        // Calculate damage
        int damage = effect.flat_damage;
        if (effect.percent_damage > 0) {
            damage += (stats_.max_hit_points * effect.percent_damage) / 100;
        }

        // Apply stack multiplier
        damage *= effect.stack_count;
        damage = std::max(1, damage);

        // Apply resistance based on damage type
        int resistance = 0;
        if (effect.damage_type == "poison") {
            resistance = stats_.resistance_poison;
        } else if (effect.damage_type == "fire") {
            resistance = stats_.resistance_fire;
        } else if (effect.damage_type == "cold") {
            resistance = stats_.resistance_cold;
        } else if (effect.damage_type == "acid") {
            resistance = stats_.resistance_acid;
        } else if (effect.damage_type == "lightning" || effect.damage_type == "shock") {
            resistance = stats_.resistance_lightning;
        }

        if (resistance > 0) {
            int max_resist = effect.max_resistance;
            resistance = std::min(resistance, max_resist);
            damage = damage * (100 - resistance) / 100;
            damage = std::max(1, damage);
        }

        // Apply damage
        result.total_damage += damage;
        result.damage_by_type.emplace_back(effect.damage_type, damage);
        stats_.hit_points -= damage;

        // Send damage message
        send_message(fmt::format("The {} courses through you! ({} damage)\r\n",
                                 effect.damage_type, damage));

        if (stats_.hit_points <= 0) {
            stats_.hit_points = 0;
            result.died = true;
            send_message(fmt::format("The {} has claimed your life!\r\n", effect.damage_type));
            return result;  // Stop processing, we're dead
        }

        // Decrement duration
        effect.decrement_duration();
        if (effect.is_expired()) {
            expired_indices.push_back(i);
        }
    }

    // Remove expired effects (reverse order to preserve indices)
    std::sort(expired_indices.begin(), expired_indices.end(), std::greater<size_t>());
    for (size_t idx : expired_indices) {
        result.expired_effects.push_back(dot_effects_[idx].damage_type);
        send_message(fmt::format("The {} effect wears off.\r\n", dot_effects_[idx].damage_type));
        dot_effects_.erase(dot_effects_.begin() + static_cast<ptrdiff_t>(idx));
    }

    // Update flags
    update_dot_flags();

    return result;
}

void Actor::update_dot_flags() {
    // Set Poison flag if any poison DoT is active
    bool has_poison = std::any_of(dot_effects_.begin(), dot_effects_.end(),
        [](const fiery::DotEffect& e) {
            return e.cure_category == "poison";
        });
    set_flag(ActorFlag::Poison, has_poison);

    // Set On_Fire flag if any fire DoT is active
    bool has_fire = std::any_of(dot_effects_.begin(), dot_effects_.end(),
        [](const fiery::DotEffect& e) {
            return e.cure_category == "fire";
        });
    set_flag(ActorFlag::On_Fire, has_fire);
}

// ============================================================================
// Data-Driven HoT Effect System Implementation
// ============================================================================

void Actor::add_hot_effect(const fiery::HotEffect& effect) {
    // Check for stacking
    if (effect.stackable) {
        // Find existing effect of same type
        for (auto& existing : hot_effects_) {
            if (existing.ability_id == effect.ability_id &&
                existing.effect_id == effect.effect_id) {
                if (existing.stack_count < existing.max_stacks) {
                    existing.stack_count++;
                    // Refresh duration
                    existing.remaining_ticks = effect.remaining_ticks;
                    Log::game()->debug("HoT effect {} stacked to {} on {}",
                                       effect.heal_type, existing.stack_count, name());
                }
                return;
            }
        }
    }

    // Add new effect
    hot_effects_.push_back(effect);
    Log::game()->debug("HoT effect {} applied to {}",
                       effect.heal_type, name());
}

void Actor::remove_hot_effects_by_category(const std::string& dispel_category) {
    auto it = std::remove_if(hot_effects_.begin(), hot_effects_.end(),
        [&dispel_category](const fiery::HotEffect& e) {
            return e.matches_dispel_category(dispel_category);
        });
    hot_effects_.erase(it, hot_effects_.end());
}

bool Actor::has_hot_effect(const std::string& hot_category) const {
    return std::any_of(hot_effects_.begin(), hot_effects_.end(),
        [&hot_category](const fiery::HotEffect& e) {
            return e.matches_dispel_category(hot_category);
        });
}

bool Actor::is_regen_boosted_by_hot() const {
    return std::any_of(hot_effects_.begin(), hot_effects_.end(),
        [](const fiery::HotEffect& e) {
            return e.boosts_regen;
        });
}

int Actor::get_hot_regen_boost() const {
    int total_boost = 0;
    for (const auto& effect : hot_effects_) {
        total_boost += effect.regen_boost;
    }
    return total_boost;  // No cap - can stack to high values
}

fiery::HotTickResult Actor::process_hot_effects() {
    fiery::HotTickResult result;
    std::vector<size_t> expired_indices;

    for (size_t i = 0; i < hot_effects_.size(); ++i) {
        auto& effect = hot_effects_[i];

        // Advance tick counter
        effect.advance_tick();

        // Check if it's time to apply healing
        if (!effect.should_tick()) continue;
        effect.reset_tick_counter();

        // Calculate healing
        int healing = effect.flat_heal;
        if (effect.percent_heal > 0) {
            healing += (stats_.max_hit_points * effect.percent_heal) / 100;
        }

        // Apply stack multiplier
        healing *= effect.stack_count;
        healing = std::max(1, healing);

        // Don't overheal
        int max_heal = stats_.max_hit_points - stats_.hit_points;
        if (healing > max_heal) {
            healing = max_heal;
        }

        // Apply healing
        if (healing > 0) {
            result.total_healing += healing;
            stats_.hit_points += healing;

            // Send healing message
            send_message(fmt::format("You feel {} energy flow through you. (+{} HP)\r\n",
                                     effect.heal_type, healing));
        }

        // Decrement duration
        effect.decrement_duration();
        if (effect.is_expired()) {
            expired_indices.push_back(i);
        }
    }

    // Remove expired effects (reverse order to preserve indices)
    std::sort(expired_indices.begin(), expired_indices.end(), std::greater<size_t>());
    for (size_t idx : expired_indices) {
        result.expired_effects.push_back(hot_effects_[idx].heal_type);
        send_message(fmt::format("The {} effect fades away.\r\n", hot_effects_[idx].heal_type));
        hot_effects_.erase(hot_effects_.begin() + static_cast<ptrdiff_t>(idx));
    }

    return result;
}

// Mobile Implementation

Mobile::Mobile(EntityId id, std::string_view name, int level)
    : Actor(id, name) {
    initialize_for_level(level);
}

Result<std::unique_ptr<Mobile>> Mobile::create(EntityId id, std::string_view name, int level) {
    if (!id.is_valid()) {
        return std::unexpected(Errors::InvalidArgument("id", "must be valid"));
    }

    if (name.empty()) {
        return std::unexpected(Errors::InvalidArgument("name", "cannot be empty"));
    }

    if (level < MIN_LEVEL || level > MAX_LEVEL) {
        return std::unexpected(Errors::InvalidArgument("level",
            fmt::format("must be between {} and {}", MIN_LEVEL, MAX_LEVEL)));
    }

    auto mobile = std::unique_ptr<Mobile>(new Mobile(id, name, level));

    // Parse keywords from name (similar to Object::create)
    auto keywords = EntityUtils::parse_keyword_list(name);
    if (!keywords.empty()) {
        mobile->set_keywords(keywords);
    }

    TRY(mobile->validate());

    return mobile;
}

Result<std::unique_ptr<Mobile>> Mobile::from_json(const nlohmann::json& json) {
    // Implementation similar to other from_json methods
    // This would parse mobile-specific fields like aggressive_, aggression_level_
    // For now, return a basic implementation
    try {
        auto base_result = Entity::from_json(json);
        if (!base_result) {
            return std::unexpected(base_result.error());
        }

        auto base_entity = std::move(base_result.value());

        int level = json.contains("level") ? json["level"].get<int>() : 1;
        // Handle legacy level 0 by converting to level 1
        if (level <= 0) level = 1;
        auto mobile = std::unique_ptr<Mobile>(new Mobile(base_entity->id(), base_entity->name(), level));

        // Copy base properties
        mobile->set_keywords(base_entity->keywords());
        mobile->set_description(base_entity->description());
        mobile->set_short_description(base_entity->short_description());

        // Parse mobile-specific properties
        if (json.contains("aggro_condition")) {
            auto cond_val = json["aggro_condition"];
            if (!cond_val.is_null() && cond_val.is_string()) {
                mobile->set_aggro_condition(cond_val.get<std::string>());
            }
        }

        // Parse alignment
        if (json.contains("alignment")) {
            int alignment = json["alignment"].get<int>();
            mobile->stats().alignment = alignment;
        }

        // Parse mob flags (array of strings or comma-separated string)
        if (json.contains("mob_flags")) {
            try {
                // Helper lambda to process a single flag name
                auto process_flag = [&mobile](const std::string& flag_name) {
                    // Handle MobFlags that don't map to ActorFlags
                    if (flag_name == "TEACHER") {
                        mobile->set_teacher(true);
                    } else if (auto flag = ActorUtils::parse_flag(flag_name)) {
                        // Standard ActorFlag
                        mobile->set_flag(flag.value(), true);
                    }
                };

                if (json["mob_flags"].is_array()) {
                    // Handle array format (modern JSON structure)
                    for (const auto& flag_json : json["mob_flags"]) {
                        process_flag(flag_json.get<std::string>());
                    }
                } else {
                    // Handle comma-separated string format (legacy)
                    std::string flags_str = json["mob_flags"].get<std::string>();
                    if (!flags_str.empty()) {
                        std::istringstream iss(flags_str);
                        std::string flag_name;
                        while (std::getline(iss, flag_name, ',')) {
                            flag_name = std::string(trim(flag_name));
                            process_flag(flag_name);
                        }
                    }
                }
            } catch (const nlohmann::json::exception& e) {
                throw std::runtime_error("mob_flags parsing error: " + std::string(e.what()));
            }
        }

        // Parse effect flags (array of strings or comma-separated string)
        if (json.contains("effect_flags")) {
            try {
                if (json["effect_flags"].is_array()) {
                    // Handle array format (modern JSON structure)
                    for (const auto& flag_json : json["effect_flags"]) {
                        std::string flag_name = flag_json.get<std::string>();
                        if (auto flag = ActorUtils::parse_flag(flag_name)) {
                            mobile->set_flag(flag.value(), true);
                        }
                    }
                } else {
                    // Handle comma-separated string format (legacy)
                    std::string effects_str = json["effect_flags"].get<std::string>();
                    if (!effects_str.empty()) {
                        std::istringstream iss(effects_str);
                        std::string flag_name;
                        while (std::getline(iss, flag_name, ',')) {
                            flag_name = std::string(trim(flag_name));

                            if (auto flag = ActorUtils::parse_flag(flag_name)) {
                                mobile->set_flag(flag.value(), true);
                            }
                        }
                    }
                }
            } catch (const nlohmann::json::exception& e) {
                throw std::runtime_error("effect_flags parsing error: " + std::string(e.what()));
            }
        }

        // Parse stats object
        if (json.contains("stats")) {
            auto stats_result = Stats::from_json(json["stats"]);
            if (stats_result) {
                mobile->stats() = stats_result.value();
            }
        }

        // Parse individual stat fields (from flat JSON format)
        Stats& stats = mobile->stats();
        if (json.contains("level")) {
            int parsed_level = json["level"].get<int>();
            if (parsed_level <= 0) parsed_level = 1;  // Handle level 0
            stats.level = parsed_level;
        }
        if (json.contains("hp_dice")) {
            // Handle dice notation like {"num": 20, "size": 8, "bonus": 100}
            const auto& hp_dice = json["hp_dice"];
            if (hp_dice.contains("bonus") && hp_dice["bonus"].get<int>() > 0) {
                stats.max_hit_points = hp_dice["bonus"].get<int>();
                stats.hit_points = stats.max_hit_points;
            }
        }
        // Convert legacy AC to new armor_rating (100 - AC)
        if (json.contains("ac")) {
            stats.armor_rating = std::max(0, 100 - json["ac"].get<int>());
        }
        // Convert legacy hit_roll to new accuracy
        if (json.contains("hit_roll")) {
            stats.accuracy = json["hit_roll"].get<int>();
        }
        // Convert legacy damage_dice bonus to new attack_power
        if (json.contains("damage_dice")) {
            const auto& dmg_dice = json["damage_dice"];
            if (dmg_dice.contains("bonus")) {
                stats.attack_power = dmg_dice["bonus"].get<int>();
            }
        }
        // Also load new stats if present
        if (json.contains("accuracy")) stats.accuracy = json["accuracy"].get<int>();
        if (json.contains("attack_power")) stats.attack_power = json["attack_power"].get<int>();
        if (json.contains("evasion")) stats.evasion = json["evasion"].get<int>();
        if (json.contains("armor_rating")) stats.armor_rating = json["armor_rating"].get<int>();

        // Parse money object - handle both string and number values
        if (json.contains("money")) {
            const auto& money = json["money"];
            long total_copper = 0;

            // Helper lambda to parse money value (string or number)
            auto parse_money_value = [](const nlohmann::json& value) -> long {
                long result = 0;
                if (value.is_string()) {
                    result = std::stol(value.get<std::string>());
                } else if (value.is_number()) {
                    result = value.get<long>();
                }
                // Treat negative values as 0 (common in legacy data for "no money")
                return std::max(0L, result);
            };

            if (money.contains("copper")) {
                total_copper += parse_money_value(money["copper"]);
            }
            if (money.contains("silver")) {
                total_copper += parse_money_value(money["silver"]) * SILVER_TO_COPPER;
            }
            if (money.contains("gold")) {
                total_copper += parse_money_value(money["gold"]) * GOLD_TO_COPPER;
            }
            if (money.contains("platinum")) {
                total_copper += parse_money_value(money["platinum"]) * PLATINUM_TO_COPPER;
            }
            stats.gold = total_copper;
        }

        // Initialize mobile for its level
        mobile->initialize_for_level(stats.level);

        TRY(mobile->validate());

        return mobile;

    } catch (const nlohmann::json::exception& e) {
        return std::unexpected(Errors::ParseError("Mobile JSON parsing error", e.what()));
    }
}

void Mobile::send_message(std::string_view message) {
    // NPCs don't have direct output, but could log for debugging
    Log::debug("Mobile {} would send: {}", name(), message);
}

void Mobile::receive_message(std::string_view message) {
    received_messages_.emplace_back(message);

    // Keep only the last N messages to avoid memory bloat
    if (received_messages_.size() > MAX_RECEIVED_MESSAGES) {
        received_messages_.erase(received_messages_.begin());
    }
}

std::shared_ptr<Container> Mobile::die() {
    auto room = current_room();
    if (!room) {
        Log::error("Mobile::die() called but mobile {} has no room", name());
        return nullptr;
    }

    // Set position to dead
    set_position(Position::Dead);

    // Calculate corpse capacity based on items to transfer
    auto equipped_items = equipment().get_all_equipped();
    auto inv_items = inventory().get_all_items();
    int corpse_capacity = static_cast<int>(equipped_items.size() + inv_items.size() + 5); // +5 buffer

    Log::info("Mobile::die() - {} has {} equipped items and {} inventory items",
              name(), equipped_items.size(), inv_items.size());

    // Create corpse as a Container to hold items
    EntityId corpse_id{id().value() + 100000}; // Add offset to avoid ID conflicts
    std::string corpse_name = fmt::format("the corpse of {}", display_name());
    auto corpse_result = Container::create(corpse_id, corpse_name, corpse_capacity, ObjectType::Corpse);

    std::shared_ptr<Container> created_corpse = nullptr;

    if (corpse_result) {
        auto corpse = std::shared_ptr<Container>(corpse_result.value().release());
        created_corpse = corpse;
        corpse->set_description(fmt::format("The lifeless body of {} lies here, still and cold.", display_name()));

        // Mark corpse as non-persistent (dynamic object with no database entry)
        corpse->set_flag(ObjectFlag::Temporary);

        // Corpses cannot be picked up and carried
        corpse->set_can_take(false);

        // Add keywords: "corpse" plus all keywords from the dead actor
        corpse->add_keyword("corpse");
        for (const auto& kw : keywords()) {
            corpse->add_keyword(kw);
        }

        // Transfer equipment to corpse
        for (auto& item : equipped_items) {
            if (item) {
                Log::debug("Transferring equipment '{}' (id {}) to corpse", item->display_name(), item->id());
                equipment().unequip_item(item->id());
                auto result = corpse->add_item(item);
                if (!result.has_value()) {
                    Log::warn("Failed to add equipment '{}' to corpse: {}", item->display_name(), result.error().message);
                    room->add_object(item); // Drop in room if corpse full
                } else {
                    Log::debug("Successfully added '{}' to corpse", item->display_name());
                }
            }
        }

        // Transfer inventory to corpse
        for (auto& item : inv_items) {
            if (item) {
                inventory().remove_item(item->id());
                auto result = corpse->add_item(item);
                if (!result.has_value()) {
                    Log::warn("Failed to add inventory '{}' to corpse, dropping in room", item->display_name());
                    room->add_object(item); // Drop in room if corpse full
                }
            }
        }

        // Create coins object if mob had money
        // Note: stats().gold stores total wealth in copper value for mobs
        long total_coins = stats().gold;
        if (total_coins > 0) {
            // Generate a descriptive name based on the amount
            std::string coins_name;
            if (total_coins >= 1000) {
                coins_name = "a large pile of coins";
            } else if (total_coins >= 100) {
                coins_name = "a pile of coins";
            } else if (total_coins >= 10) {
                coins_name = "some coins";
            } else {
                coins_name = "a few coins";
            }

            EntityId coins_id{id().value() + 200000}; // Different offset than corpse
            auto coins_result = Object::create(coins_id, coins_name, ObjectType::Money);
            if (coins_result) {
                auto coins = std::shared_ptr<Object>(coins_result.value().release());
                coins->set_value(static_cast<int>(total_coins));

                // Mark coins as non-persistent (dynamic object with no database entry)
                coins->set_flag(ObjectFlag::Temporary);

                coins->add_keyword("coins");
                coins->add_keyword("money");
                coins->add_keyword("gold"); // Generic keyword for finding

                // Convert total copper to coin breakdown for description
                long remaining = total_coins;
                int plat = static_cast<int>(remaining / 1000);
                remaining %= 1000;
                int gold_coins = static_cast<int>(remaining / 100);
                remaining %= 100;
                int silver_coins = static_cast<int>(remaining / 10);
                int copper_coins = static_cast<int>(remaining % 10);

                // Generate description showing coin amounts
                std::string coin_desc;
                if (plat > 0) coin_desc += fmt::format("{} platinum, ", plat);
                if (gold_coins > 0) coin_desc += fmt::format("{} gold, ", gold_coins);
                if (silver_coins > 0) coin_desc += fmt::format("{} silver, ", silver_coins);
                if (copper_coins > 0) coin_desc += fmt::format("{} copper, ", copper_coins);
                // Remove trailing ", "
                if (!coin_desc.empty()) {
                    coin_desc = coin_desc.substr(0, coin_desc.length() - 2);
                }
                coins->set_description(fmt::format("A pile of coins: {}.", coin_desc));

                auto result = corpse->add_item(coins);
                if (result.has_value()) {
                    Log::debug("Added {} coins (value: {}) to corpse", coins_name, total_coins);
                } else {
                    Log::warn("Failed to add coins to corpse, dropping in room");
                    room->add_object(coins);
                }
            }
        }

        // Add corpse to room
        room->add_object(corpse);

        // Handle falling if in flying room
        WorldManager::instance().check_and_handle_object_falling(corpse, room);

        Log::info("Created corpse for mobile {} with {} items in room {}",
                  name(), corpse->current_capacity(), room->id());
    } else {
        Log::error("Failed to create corpse for mobile {}: {}", name(), corpse_result.error().message);
    }

    // Remove from room and despawn from world
    room->remove_actor(id());
    WorldManager::instance().despawn_mobile(id());

    Log::info("Mobile {} died and was removed from world", name());

    return created_corpse;
}

int Mobile::calculate_hp_from_dice() {
    // Roll HP dice: num d size + bonus
    // For example: 50d20+20783 means roll 50 d20s and add 20783
    int total = hp_dice_bonus_;

    // Roll each die
    for (int i = 0; i < hp_dice_num_; ++i) {
        // Random roll from 1 to size (use average for consistency in spawning)
        // Using average: (1 + size) / 2
        total += (1 + hp_dice_size_) / 2;
    }

    // Set the HP
    stats().max_hit_points = total;
    stats().hit_points = total;

    return total;
}

void Mobile::set_stance(std::string_view s) {
    // Convert SCREAMING_SNAKE_CASE database value to PascalCase enum
    std::string pascal_name;
    pascal_name.reserve(s.size());
    bool capitalize_next = true;
    for (char c : s) {
        if (c == '_') {
            capitalize_next = true;
        } else if (capitalize_next) {
            pascal_name += static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
            capitalize_next = false;
        } else {
            pascal_name += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        }
    }

    auto stance = magic_enum::enum_cast<db::Stance>(pascal_name);
    if (stance) {
        stance_ = *stance;
    } else {
        // Default to Alert if unknown
        stance_ = db::Stance::Alert;
    }
}

void Mobile::initialize_for_level(int level) {
    Stats& s = stats();
    s.level = level;
    s.experience = ActorUtils::experience_for_level(level);

    // Set reasonable stats for NPCs
    s.max_hit_points = ActorUtils::calculate_hit_points(level, s.constitution);
    s.hit_points = s.max_hit_points;
    s.max_mana = ActorUtils::calculate_mana(level, s.intelligence);
    s.mana = s.max_mana;
    s.max_stamina = ActorUtils::calculate_stamina(level, s.constitution);
    s.stamina = s.max_stamina;
}

std::string_view Mobile::effect_descriptor() const {
    // Return a visible effect descriptor based on active effects
    // Priority order: most visually striking first
    if (has_effect(EffectFlag::Sanctuary)) {
        return "glowing";
    }
    if (has_effect(EffectFlag::Fly)) {
        return "flying";
    }
    if (has_effect(EffectFlag::Berserk)) {
        return "raging";
    }
    if (has_effect(EffectFlag::StoneSkin)) {
        return "stone-skinned";
    }
    if (has_effect(EffectFlag::Poison)) {
        return "sickly";
    }
    if (has_effect(EffectFlag::Blur)) {
        return "blurred";
    }
    if (has_effect(EffectFlag::Haste)) {
        return "swift";
    }
    if (has_effect(EffectFlag::Glory)) {
        return "glorious";
    }
    // Invisible is handled separately in display_name_for_observer
    return "";
}

std::string Mobile::display_name(bool with_article) const {
    // If we don't have base_name set, fall back to Entity behavior
    if (base_name_.empty()) {
        return Entity::display_name(with_article);
    }

    // Get effect descriptor (glowing, flying, etc)
    std::string_view effect = effect_descriptor();

    std::string result;

    if (with_article) {
        // Determine the article to use
        // article_ semantics:
        //   - nullopt: calculate a/an based on first letter of next word
        //   - "": no article
        //   - "the", "some": specific article
        if (article_.has_value()) {
            if (!article_->empty()) {
                // Explicit article like "the" or "some"
                result = *article_;
                result += ' ';
            }
            // If article is empty string, no article is added
        } else {
            // Calculate a/an based on first letter of next word
            // The next word is either the effect descriptor or the base_name
            std::string_view first_word = effect.empty() ? base_name_ : effect;

            // Find first letter (skip any color markup tags like <red>)
            char first_letter = '\0';
            size_t i = 0;
            while (i < first_word.size()) {
                if (first_word[i] == '<') {
                    // Skip the tag
                    while (i < first_word.size() && first_word[i] != '>') {
                        ++i;
                    }
                    ++i;  // Skip the '>'
                } else if (std::isalpha(static_cast<unsigned char>(first_word[i]))) {
                    first_letter = static_cast<char>(std::tolower(static_cast<unsigned char>(first_word[i])));
                    break;
                } else {
                    ++i;
                }
            }

            // Use 'an' before vowels, 'a' otherwise
            bool use_an = (first_letter == 'a' || first_letter == 'e' ||
                           first_letter == 'i' || first_letter == 'o' || first_letter == 'u');
            result = use_an ? "an " : "a ";
        }
    }

    // Add effect descriptor if present
    if (!effect.empty()) {
        result += effect;
        result += ' ';
    }

    // Add the base name
    result += base_name_;

    return result;
}

// Player Implementation

Player::Player(EntityId id, std::string_view name)
    : Actor(id, name) {
    // Set sensible defaults for new players
    set_player_flag(PlayerFlag::AutoExit);   // Show exits when entering rooms
    set_player_flag(PlayerFlag::AutoLoot);   // Auto-loot corpses
    set_player_flag(PlayerFlag::AutoGold);   // Auto-collect gold from corpses
}

Result<std::unique_ptr<Player>> Player::create(EntityId id, std::string_view name) {
    if (!id.is_valid()) {
        return std::unexpected(Errors::InvalidArgument("id", "must be valid"));
    }

    if (name.empty()) {
        return std::unexpected(Errors::InvalidArgument("name", "cannot be empty"));
    }

    auto player = std::unique_ptr<Player>(new Player(id, name));

    TRY(player->validate());

    return player;
}

Result<std::unique_ptr<Player>> Player::from_json(const nlohmann::json& json) {
    // Similar implementation to Mobile::from_json
    try {
        auto base_result = Entity::from_json(json);
        if (!base_result) {
            return std::unexpected(base_result.error());
        }

        auto base_entity = std::move(base_result.value());
        auto player = std::unique_ptr<Player>(new Player(base_entity->id(), base_entity->name()));

        // Copy base properties
        player->set_keywords(base_entity->keywords());
        player->set_short_description(base_entity->short_description());

        // Load Stats from Actor::to_json
        if (json.contains("stats")) {
            auto stats_result = Stats::from_json(json["stats"]);
            if (stats_result) {
                player->stats() = stats_result.value();
            } else {
                Log::warn("Failed to load stats for player '{}': {}",
                         player->name(), stats_result.error().message);
            }
        }

        // Load position from Actor::to_json
        if (json.contains("position")) {
            auto pos_str = json["position"].get<std::string>();
            auto pos = magic_enum::enum_cast<Position>(pos_str);
            if (pos.has_value()) {
                player->set_position(pos.value());
            }
        }

        // Load actor flags from Actor::to_json (different from player_flags)
        if (json.contains("flags")) {
            for (const auto& flag_name : json["flags"]) {
                auto flag = magic_enum::enum_cast<ActorFlag>(flag_name.get<std::string>());
                if (flag.has_value()) {
                    player->set_flag(flag.value(), true);
                }
            }
        }

        // Parse player-specific properties
        if (json.contains("account")) {
            player->set_account(json["account"].get<std::string>());
        }

        if (json.contains("god_level")) {
            player->set_god_level(json["god_level"].get<int>());
        }

        if (json.contains("player_class")) {
            player->set_class(json["player_class"].get<std::string>());
        }

        if (json.contains("race")) {
            player->set_race(json["race"].get<std::string>());
        }

        if (json.contains("gender")) {
            player->set_gender(json["gender"].get<std::string>());
        }

        if (json.contains("start_room")) {
            auto start_room_id = json["start_room"].get<uint64_t>();
            player->set_start_room(EntityId{start_room_id});
        }

        if (json.contains("recall_room")) {
            auto recall_room_id = json["recall_room"].get<uint64_t>();
            player->set_recall_room(EntityId{recall_room_id});
        }

        // Load spell slots if present
        if (json.contains("spell_slots")) {
            auto spell_slots_result = SpellSlots::from_json(json["spell_slots"]);
            if (spell_slots_result) {
                player->spell_slots() = spell_slots_result.value();
            } else {
                Log::warn("Failed to load spell slots for player '{}': {}",
                         player->name(), spell_slots_result.error().message);
            }
        }

        // Load player preferences and flags
        if (json.contains("prompt")) {
            player->set_prompt(json["prompt"].get<std::string>());
        }

        if (json.contains("title")) {
            player->set_title(json["title"].get<std::string>());
        }

        if (json.contains("description")) {
            player->set_description(json["description"].get<std::string>());
        }

        if (json.contains("wimpy_threshold")) {
            player->set_wimpy_threshold(json["wimpy_threshold"].get<int>());
        }

        if (json.contains("player_flags")) {
            player->set_player_flags_from_strings(json["player_flags"].get<std::vector<std::string>>());
        }

        // Load wallet and bank
        if (json.contains("wallet")) {
            player->wallet() = fiery::Money::from_json(json["wallet"]);
        }

        if (json.contains("bank")) {
            player->bank() = fiery::Money::from_json(json["bank"]);
        }

        // Load active effects
        if (json.contains("active_effects")) {
            for (const auto& eff_json : json["active_effects"]) {
                ActiveEffect effect;
                effect.effect_id = eff_json.value("effect_id", 0);
                effect.name = eff_json.value("name", "");
                effect.source = eff_json.value("source", "");
                auto flag = magic_enum::enum_cast<ActorFlag>(eff_json.value("flag", "None"));
                effect.flag = flag.value_or(ActorFlag::None);
                effect.duration_hours = eff_json.value("duration_hours", 0.0);
                effect.modifier_value = eff_json.value("modifier_value", 0);
                effect.modifier_stat = eff_json.value("modifier_stat", "");
                effect.applied_at = std::chrono::steady_clock::now();
                player->add_effect(effect);
            }
        }

        // Load DoT effects
        if (json.contains("dot_effects")) {
            for (const auto& dot_json : json["dot_effects"]) {
                fiery::DotEffect dot;
                dot.ability_id = dot_json.value("ability_id", 0);
                dot.effect_id = dot_json.value("effect_id", 0);
                dot.effect_type = dot_json.value("effect_type", "");
                dot.damage_type = dot_json.value("damage_type", "");
                dot.cure_category = dot_json.value("cure_category", "");
                dot.potency = dot_json.value("potency", 5);
                dot.flat_damage = dot_json.value("flat_damage", 0);
                dot.percent_damage = dot_json.value("percent_damage", 0);
                dot.blocks_regen = dot_json.value("blocks_regen", false);
                dot.reduces_regen = dot_json.value("reduces_regen", 0);
                dot.remaining_ticks = dot_json.value("remaining_ticks", -1);
                dot.tick_interval = dot_json.value("tick_interval", 1);
                dot.ticks_since_last = dot_json.value("ticks_since_last", 0);
                dot.source_actor_id = dot_json.value("source_actor_id", "");
                dot.source_level = dot_json.value("source_level", 1);
                dot.stack_count = dot_json.value("stack_count", 1);
                dot.max_stacks = dot_json.value("max_stacks", 1);
                dot.stackable = dot_json.value("stackable", false);
                player->add_dot_effect(dot);
            }
        }

        // Load HoT effects
        if (json.contains("hot_effects")) {
            for (const auto& hot_json : json["hot_effects"]) {
                fiery::HotEffect hot;
                hot.ability_id = hot_json.value("ability_id", 0);
                hot.effect_id = hot_json.value("effect_id", 0);
                hot.effect_type = hot_json.value("effect_type", "");
                hot.heal_type = hot_json.value("heal_type", "");
                hot.hot_category = hot_json.value("hot_category", "");
                hot.flat_heal = hot_json.value("flat_heal", 0);
                hot.percent_heal = hot_json.value("percent_heal", 0);
                hot.boosts_regen = hot_json.value("boosts_regen", false);
                hot.regen_boost = hot_json.value("regen_boost", 0);
                hot.remaining_ticks = hot_json.value("remaining_ticks", -1);
                hot.tick_interval = hot_json.value("tick_interval", 1);
                hot.ticks_since_last = hot_json.value("ticks_since_last", 0);
                hot.source_actor_id = hot_json.value("source_actor_id", "");
                hot.source_level = hot_json.value("source_level", 1);
                hot.stack_count = hot_json.value("stack_count", 1);
                hot.max_stacks = hot_json.value("max_stacks", 1);
                hot.stackable = hot_json.value("stackable", false);
                player->add_hot_effect(hot);
            }
        }

        // Initialize spell slots based on class and level
        player->initialize_spell_slots();

        TRY(player->validate());

        return player;

    } catch (const nlohmann::json::exception& e) {
        return std::unexpected(Errors::ParseError("Player JSON parsing error", e.what()));
    }
}

std::string Player::display_name(bool /* with_article */) const {
    // For players, display name is always their actual name
    return std::string{name()};
}

void Player::send_message(std::string_view message) {
    output_queue_.emplace_back(message);

    // If we have an output interface (e.g., PlayerConnection), send immediately
    // Color processing happens at the PlayerConnection level for ALL output paths
    if (output_) {
        output_->send_message(message);
    }
}

void Player::receive_message(std::string_view message) {
    send_message(message); // Players receive messages by sending them to output
}

void Player::start_composing(std::shared_ptr<ComposerSystem> composer) {
    if (active_composer_) {
        Log::warn("Player {} starting new composer while one is already active", name());
    }
    active_composer_ = std::move(composer);
    if (active_composer_) {
        active_composer_->start();
    }
}

void Player::stop_composing() {
    active_composer_.reset();
}

void Player::interrupt_composing(std::string_view reason) {
    if (!active_composer_) return;

    // Notify player that their composition was interrupted
    if (reason.empty()) {
        send_message("\nYour text composition has been interrupted!");
    } else {
        send_message(fmt::format("\nYour text composition has been interrupted: {}", reason));
    }

    // Get current buffer and save it for 'redo' command recovery
    auto lines = active_composer_->lines();
    if (!lines.empty()) {
        set_redo_buffer(std::move(lines));
        send_message(fmt::format("({} line{} of text was saved. Use 'redo' to recover.)",
            redo_buffer().size(), redo_buffer().size() == 1 ? "" : "s"));
    }

    active_composer_.reset();
}

std::shared_ptr<Container> Player::die() {
    // Players become ghosts - corpse is created when they use 'release' command
    set_position(Position::Ghost);
    return nullptr;  // Player corpse is created on release, not death
}

nlohmann::json Player::get_vitals_gmcp() const {
    const auto& stats = this->stats();

    // Calculate experience percentage to next level
    long exp_for_current = ActorUtils::experience_for_level(stats.level);
    long exp_for_next = ActorUtils::experience_for_level(stats.level + 1);
    int exp_percent = (exp_for_next > exp_for_current)
        ? static_cast<int>(100 * (stats.experience - exp_for_current) / (exp_for_next - exp_for_current))
        : 0;

    // Get position name
    std::string position_name = std::string(ActorUtils::get_position_name(position()));

    // Build effects array
    nlohmann::json effects = nlohmann::json::array();
    for (const auto& effect : active_effects()) {
        if (effect.duration_hours >= 0) {
            nlohmann::json effect_obj;
            effect_obj["name"] = effect.name;
            effect_obj["duration"] = static_cast<int>(effect.duration_hours + 1);
            effects.push_back(effect_obj);
        }
    }

    // Build combat info
    nlohmann::json combat = nlohmann::json::object();
    if (is_fighting()) {
        auto target = get_fighting_target();
        if (target) {
            int target_hp_percent = (target->stats().max_hit_points > 0)
                ? (100 * target->stats().hit_points / target->stats().max_hit_points)
                : 0;
            nlohmann::json opponent_obj;
            opponent_obj["name"] = target->display_name();
            opponent_obj["hp_percent"] = target_hp_percent;
            combat["opponent"] = opponent_obj;

            // Check if opponent is fighting someone else (tank)
            if (target->is_fighting()) {
                auto tank = target->get_fighting_target();
                if (tank && tank.get() != this) {
                    nlohmann::json tank_obj;
                    tank_obj["name"] = tank->display_name();
                    tank_obj["hp"] = tank->stats().hit_points;
                    tank_obj["max_hp"] = tank->stats().max_hit_points;
                    combat["tank"] = tank_obj;
                }
            }
        }
    }

    // Build spell slots (available per circle)
    nlohmann::json spell_slots_json = nlohmann::json::object();
    for (int circle = 1; circle <= 9; ++circle) {
        auto [current, max_slots] = spell_slots().get_slot_info(circle);
        if (max_slots > 0) {
            spell_slots_json[std::to_string(circle)] = nlohmann::json{
                {"available", current},
                {"max", max_slots}
            };
        }
    }

    // Build the comprehensive Char GMCP data (matches legacy format)
    return {
        {"name", std::string(name())},
        {"class", player_class_},
        {"race", std::string(race())},
        {"exp_percent", exp_percent},
        {"alignment", stats.alignment},
        {"position", position_name},
        {"hiddenness", 0},  // TODO: Add hiddenness tracking when stealth system is implemented
        {"level", stats.level},
        {"Vitals", {
            {"hp", stats.hit_points},
            {"max_hp", stats.max_hit_points},
            {"mv", stats.stamina},
            {"max_mv", stats.max_stamina}
        }},
        {"Worth", {
            {"Carried", {
                {"platinum", wallet_.platinum()},
                {"gold", wallet_.gold()},
                {"silver", wallet_.silver()},
                {"copper", wallet_.copper()}
            }},
            {"Bank", {
                {"platinum", bank_.platinum()},
                {"gold", bank_.gold()},
                {"silver", bank_.silver()},
                {"copper", bank_.copper()}
            }}
        }},
        {"Effects", effects},
        {"Combat", combat},
        {"SpellSlots", spell_slots_json}
    };
}

nlohmann::json Player::get_status_gmcp() const {
    const auto& stats = this->stats();
    auto room = current_room();
    return {
        {"name", name()},
        {"level", stats.level},
        {"class", player_class_},
        {"race", std::string(race())},
        {"room", room ? room->id().value() : 0},
        {"gold", stats.gold}
    };
}

void Player::on_room_change(std::shared_ptr<Room> /* old_room */, std::shared_ptr<Room> /* new_room */) {
    // Send GMCP room update if player has output (connection) and supports GMCP
    if (auto output = get_output()) {
        // Try to cast to PlayerConnection to access GMCP methods
        if (auto connection = std::dynamic_pointer_cast<class PlayerConnection>(output)) {
            connection->send_room_info();
        }
    }
}

void Player::send_gmcp_vitals_update() {
    // Send GMCP vitals update if player has output (connection) and supports GMCP
    if (auto output = get_output()) {
        // Try to cast to PlayerConnection to access GMCP methods
        if (auto connection = std::dynamic_pointer_cast<class PlayerConnection>(output)) {
            connection->send_vitals();
        }
    }
}

void Player::on_position_change(Position /* old_pos */, Position /* new_pos */) {
    // Send GMCP vitals update when position changes (position is included in vitals)
    send_gmcp_vitals_update();
}

void Player::initialize_spell_slots() {
    spell_slots().initialize_for_class(player_class_, stats().level);
    Log::debug("Initialized spell slots for player '{}' (class: {}, level: {})",
              name(), player_class_, stats().level);
}

void Player::on_level_up(int /* old_level */, int /* new_level */) {
    // Update spell slots for new level
    initialize_spell_slots();

    // Send GMCP vitals update when player levels up (vitals change)
    send_gmcp_vitals_update();
}

void Player::start_meditation() {
    if (!is_meditating_) {
        is_meditating_ = true;
        Log::debug("Player '{}' started meditating", name());
    }
}

void Player::stop_meditation() {
    if (is_meditating_) {
        is_meditating_ = false;
        Log::debug("Player '{}' stopped meditating", name());
    }
}

int Player::get_spell_restore_rate() const {
    // Base rate: focus / 10 (minimum 1)
    int rate = std::max(1, stats().focus / 10);

    // Meditation doubles the restoration rate
    if (is_meditating_) {
        rate *= 2;
    }

    return rate;
}

void Player::send_to_group(std::string_view message) {
    // If we're following someone, get the leader's group
    auto leader = get_leader();
    if (leader) {
        // Send to leader (who is a Player)
        if (auto leader_player = std::dynamic_pointer_cast<Player>(leader)) {
            leader_player->send_message(std::string(message));

            // Send to all of leader's followers
            for (const auto& weak_follower : leader_player->get_followers()) {
                if (auto follower = weak_follower.lock()) {
                    if (auto follower_player = std::dynamic_pointer_cast<Player>(follower)) {
                        follower_player->send_message(std::string(message));
                    }
                }
            }
        }
    } else {
        // We are the leader or solo - send to self and all followers
        send_message(std::string(message));

        for (const auto& weak_follower : followers_) {
            if (auto follower = weak_follower.lock()) {
                if (auto follower_player = std::dynamic_pointer_cast<Player>(follower)) {
                    follower_player->send_message(std::string(message));
                }
            }
        }
    }
}

// Player flag name mapping for database persistence
namespace {
    const std::unordered_map<PlayerFlag, std::string> player_flag_names = {
        {PlayerFlag::Brief, "BRIEF"},
        {PlayerFlag::Compact, "COMPACT"},
        {PlayerFlag::NoRepeat, "NOREPEAT"},
        {PlayerFlag::AutoLoot, "AUTOLOOT"},
        {PlayerFlag::AutoGold, "AUTOGOLD"},
        {PlayerFlag::AutoSplit, "AUTOSPLIT"},
        {PlayerFlag::AutoExit, "AUTOEXIT"},
        {PlayerFlag::AutoAssist, "AUTOASSIST"},
        {PlayerFlag::Wimpy, "WIMPY"},
        {PlayerFlag::ShowDiceRolls, "SHOWDICEROLLS"},
        {PlayerFlag::Afk, "AFK"},
        {PlayerFlag::Deaf, "DEAF"},
        {PlayerFlag::NoTell, "NOTELL"},
        {PlayerFlag::NoSummon, "NOSUMMON"},
        {PlayerFlag::Quest, "QUEST"},
        {PlayerFlag::PkEnabled, "PKENABLED"},
        {PlayerFlag::Consent, "CONSENT"},
        {PlayerFlag::ColorBlind, "COLORBLIND"},
        {PlayerFlag::Msp, "MSP"},
        {PlayerFlag::MxpEnabled, "MXP"},
    };

    const std::unordered_map<std::string, PlayerFlag> player_flag_from_string = {
        {"BRIEF", PlayerFlag::Brief},
        {"COMPACT", PlayerFlag::Compact},
        {"NOREPEAT", PlayerFlag::NoRepeat},
        {"AUTOLOOT", PlayerFlag::AutoLoot},
        {"AUTOGOLD", PlayerFlag::AutoGold},
        {"AUTOSPLIT", PlayerFlag::AutoSplit},
        {"AUTOEXIT", PlayerFlag::AutoExit},
        {"AUTOASSIST", PlayerFlag::AutoAssist},
        {"WIMPY", PlayerFlag::Wimpy},
        {"SHOWDICEROLLS", PlayerFlag::ShowDiceRolls},
        {"AFK", PlayerFlag::Afk},
        {"DEAF", PlayerFlag::Deaf},
        {"NOTELL", PlayerFlag::NoTell},
        {"NOSUMMON", PlayerFlag::NoSummon},
        {"QUEST", PlayerFlag::Quest},
        {"PKENABLED", PlayerFlag::PkEnabled},
        {"CONSENT", PlayerFlag::Consent},
        {"COLORBLIND", PlayerFlag::ColorBlind},
        {"MSP", PlayerFlag::Msp},
        {"MXP", PlayerFlag::MxpEnabled},
    };
}

std::vector<std::string> Player::get_player_flags_as_strings() const {
    std::vector<std::string> flags;
    for (const auto& [flag, name] : player_flag_names) {
        if (has_player_flag(flag)) {
            flags.push_back(name);
        }
    }
    return flags;
}

void Player::set_player_flags_from_strings(const std::vector<std::string>& flags) {
    // Clear all flags first
    player_flags_.reset();

    for (const auto& flag_str : flags) {
        // Convert to uppercase for comparison
        std::string upper = flag_str;
        std::transform(upper.begin(), upper.end(), upper.begin(),
                       [](unsigned char c) { return std::toupper(c); });

        auto it = player_flag_from_string.find(upper);
        if (it != player_flag_from_string.end()) {
            set_player_flag(it->second, true);
        } else {
            Log::warn("Unknown player flag: {}", flag_str);
        }
    }
}

fiery::Money Player::loot_money_from(Mobile& mob) {
    fiery::Money loot = mob.take_all_money();
    if (!loot.is_zero()) {
        wallet_ += loot;
    }
    return loot;
}

nlohmann::json Player::to_json() const {
    nlohmann::json json = Actor::to_json();

    // Add Player-specific fields
    json["type"] = "Player";
    json["account"] = account_;
    json["god_level"] = god_level_;
    json["player_class"] = player_class_;
    json["race"] = std::string(race());
    json["gender"] = std::string(gender());
    json["start_room"] = start_room_.value();
    json["recall_room"] = recall_room_.value();
    json["player_flags"] = get_player_flags_as_strings();
    json["wimpy_threshold"] = wimpy_threshold_;
    json["title"] = title_;
    json["description"] = description_;
    json["prompt"] = prompt_;
    json["wallet"] = wallet_.to_json();
    json["bank"] = bank_.to_json();

    return json;
}

// ActorUtils Implementation

namespace ActorUtils {
    long experience_for_level(int level) {
        if (level <= MIN_LEVEL) return 0;

        // Use a progressive scale: level^EXP_LEVEL_EXPONENT * EXP_BASE_MULTIPLIER
        return static_cast<long>(std::pow(level, EXP_LEVEL_EXPONENT) * EXP_BASE_MULTIPLIER);
    }

    int calculate_hit_points(int level, int constitution) {
        int base = 8 * level;
        int con_bonus = Stats::attribute_modifier(constitution) * level;
        return std::max(level, base + con_bonus);
    }

    int calculate_mana(int level, int intelligence) {
        int base = 4 * level;
        int int_bonus = Stats::attribute_modifier(intelligence) * level;
        return std::max(0, base + int_bonus);
    }

    int calculate_stamina(int level, int constitution) {
        int base = 100 + (level * 2);
        int con_bonus = Stats::attribute_modifier(constitution) * 5;
        return std::max(50, base + con_bonus);
    }

    std::string format_stats(const Stats& stats) {
        return fmt::format("HP: {}/{}, Mana: {}/{}, Stamina: {}/{}, ACC: {}, EVA: {}, AR: {}, Level: {}",
                          stats.hit_points, stats.max_hit_points,
                          stats.mana, stats.max_mana,
                          stats.stamina, stats.max_stamina,
                          stats.accuracy, stats.evasion, stats.armor_rating, stats.level);
    }

    std::string_view get_position_name(Position position) {
        auto name = magic_enum::enum_name(position);
        return name.empty() ? "Unknown" : name;
    }

    std::optional<Position> parse_position(std::string_view position_name) {
        return magic_enum::enum_cast<Position>(position_name);
    }

    std::string_view get_flag_name(ActorFlag flag) {
        auto name = magic_enum::enum_name(flag);
        return name.empty() ? "Unknown" : name;
    }

    std::optional<ActorFlag> parse_flag(std::string_view flag_name) {
        return magic_enum::enum_cast<ActorFlag>(flag_name);
    }
}

// JSON Support Functions

void to_json(nlohmann::json& json, const Stats& stats) {
    json = stats.to_json();
}

void from_json(const nlohmann::json& json, Stats& stats) {
    auto result = Stats::from_json(json);
    if (result) {
        stats = result.value();
    } else {
        throw std::runtime_error(fmt::format("Failed to parse Stats from JSON: {}", result.error().message));
    }
}

void to_json(nlohmann::json& json, const Actor& actor) {
    json = actor.to_json();
}

void from_json(const nlohmann::json& json, std::unique_ptr<Actor>& actor) {
    // This would need to determine actor type and call appropriate from_json
    std::string type = json.contains("type") ? json["type"].get<std::string>() : "Actor";

    if (type == "Mobile") {
        auto result = Mobile::from_json(json);
        if (result) {
            actor = std::move(result.value());
        } else {
            throw std::runtime_error(fmt::format("Failed to create Mobile from JSON: {}", result.error().message));
        }
    } else if (type == "Player") {
        auto result = Player::from_json(json);
        if (result) {
            actor = std::move(result.value());
        } else {
            throw std::runtime_error(fmt::format("Failed to create Player from JSON: {}", result.error().message));
        }
    } else {
        throw std::runtime_error(fmt::format("Unknown actor type: {}", type));
    }
}

std::string Actor::get_stat_info() const {
    std::ostringstream output;

    // Cast to specific types for additional information
    auto player = dynamic_cast<const Player*>(this);
    auto mobile = dynamic_cast<const Mobile*>(this);

    // Helper to format EntityId as zone:local_id
    auto format_id = [](const EntityId& eid) -> std::string {
        return fmt::format("{}:{}", eid.zone_id(), eid.local_id());
    };

    // Vital character data - modern format with zone:local_id IDs
    std::string room_str = current_room() ? format_id(current_room()->id()) : "none";
    output << fmt::format("{} '{}'  ID: [{}], In room [{}]\n",
        player ? "PC" : (mobile ? "MOB" : "NPC"),
        name(),
        format_id(id()),
        room_str);

    // Mobile-specific data
    if (mobile) {
        // Show keywords (what players can use to target this mob) in a visible format
        auto kw = keywords();
        std::string keywords_str;
        for (size_t i = 0; i < kw.size(); ++i) {
            if (i > 0) keywords_str += ", ";
            keywords_str += fmt::format("'{}'", kw[i]);
        }
        output << fmt::format("Keywords: {}\n", keywords_str.empty() ? name() : keywords_str);
    }

    // Descriptions
    if (mobile) {
        output << fmt::format("Short Desc: {}\n", short_description().empty() ? "<None>" : short_description());
        output << fmt::format("Room Desc: {}\n", ground().empty() ? "<None>" : ground());
        output << fmt::format("Look Desc: {}\n", mobile->description().empty() ? "<None>" : mobile->description());
    } else if (player) {
        output << fmt::format("Title: {}\n", player->title().empty() ? "<None>" : player->title());
    }

    // Character stats and abilities
    const auto& stats_ = stats();

    // Get race, gender, size, life force, composition - available on all Actors via base class
    std::string race_str = std::string(race());  // Actor::race() works for both Mobile and Player
    std::string gender_str = std::string(gender());
    std::string size_str = std::string(size());
    std::string life_force_str = mobile ? std::string(mobile->life_force()) : "Life";
    std::string composition_str = mobile ? std::string(mobile->composition()) : "Flesh";

    output << fmt::format("Race: {}, Race Align: N/A, ", race_str);
    output << fmt::format("Size: {}, Gender: {}\n", size_str, gender_str);
    output << fmt::format("Life force: {}, Composition: {}\n", life_force_str, composition_str);
    output << fmt::format("Class: {}, Lev: [{:2d}], XP: [{:7}], Align: [{:4d}]\n",
        player ? player->player_class() : "NPC",
        stats_.level,
        stats_.experience,
        stats_.alignment);

    // Player-specific data
    if (player) {
        // Format time displays
        auto format_time = [](std::time_t time) -> std::string {
            if (time == 0) return "Never";
            auto tm = *std::localtime(&time);
            return fmt::format("{:02d}/{:02d}/{:4d} {:02d}:{:02d}",
                tm.tm_mon + 1, tm.tm_mday, tm.tm_year + 1900,
                tm.tm_hour, tm.tm_min);
        };

        auto creation = format_time(player->creation_time());
        auto last_logon = format_time(player->last_logon_time());
        auto play_hours = player->total_play_time().count() / SECONDS_PER_HOUR;

        output << fmt::format("Created: [{}], Last Logon: [{}], Played: [{}h]\n",
            creation, last_logon, play_hours);
        output << fmt::format("Age: [{}], Homeroom: [{}]",
            PLAYER_BASE_AGE + stats_.level,
            format_id(player->start_room()));

        // Clan/Guild information
        if (player->in_clan()) {
            output << fmt::format(", Clan: [{} ({})] Rank: [{}]",
                player->clan_name(),
                player->clan_abbreviation(),
                player->clan_rank_title().empty() ? "Member" : player->clan_rank_title());
        }
        output << "\n";
    }

    // Attributes display (similar to legacy format)
    output << fmt::format(
        "         STR   INT   WIS   DEX   CON   CHA\n"
        "ACTUAL   {:3}   {:3}   {:3}   {:3}   {:3}   {:3}\n"
        "NATURAL  {:3}   {:3}   {:3}   {:3}   {:3}   {:3}\n"
        "AFFECTED {:3}   {:3}   {:3}   {:3}   {:3}   {:3}\n",
        stats_.strength, stats_.intelligence, stats_.wisdom, stats_.dexterity, stats_.constitution, stats_.charisma,
        stats_.strength, stats_.intelligence, stats_.wisdom, stats_.dexterity, stats_.constitution, stats_.charisma,
        stats_.strength, stats_.intelligence, stats_.wisdom, stats_.dexterity, stats_.constitution, stats_.charisma);

    // Health, mana, and stamina
    output << fmt::format("HP: [{}/{}]  HP Gain: [{}] HP Regen Bonus: [{}]\n",
        stats_.hit_points, stats_.max_hit_points, 1, 0);
    output << fmt::format("Stamina: [{}/{}]  Stamina Gain: [{}]\n",
        stats_.stamina, stats_.max_stamina, 1);
    output << fmt::format("Focus: [{}]\n", stats_.mana);

    // Money (convert from copper base to coins)
    long total_copper = stats_.gold;
    long platinum = total_copper / PLATINUM_TO_COPPER; total_copper %= PLATINUM_TO_COPPER;
    long gold = total_copper / GOLD_TO_COPPER; total_copper %= GOLD_TO_COPPER;
    long silver = total_copper / SILVER_TO_COPPER; total_copper %= SILVER_TO_COPPER;
    long copper = total_copper;

    output << fmt::format(
        "Coins: [{}p / {}g / {}s / {}c], "
        "Bank: [0p / 0g / 0s / 0c]\n",
        platinum, gold, silver, copper);

    // Combat stats (new ACC/EVA system)
    output << fmt::format("Accuracy: [{:2}], Evasion: [{:2}], Attack Power: [{:2}]\n",
        stats_.accuracy, stats_.evasion, stats_.attack_power);
    output << fmt::format("Armor Rating: [{:2}], DR%: [{:2}], Spell Power: [{:2}]\n",
        stats_.armor_rating, stats_.damage_reduction_percent, stats_.spell_power);
    output << fmt::format("Saving throws: [Para: 0, Rod: 0, Petr: 0, Breath: 0, Spell: 0]\n");
    output << fmt::format("Perception: [{:4}], Stealth: [{:4}], Rage: [{:4}]\n", 0, 0, 0);

    // Position and status
    output << fmt::format("Pos: {}", magic_enum::enum_name(position()));
    if (mobile) {
        output << fmt::format(", Default Pos: {}", magic_enum::enum_name(position()));
    }
    output << fmt::format(", Fighting: <none>");
    output << "\n";

    // Player connection info
    if (player) {
        output << fmt::format("Idle: [0 tics]\n");
    }

    // Mobile combat info
    if (mobile) {
        // HP dice
        int hp_bonus = mobile->hp_dice_bonus();
        std::string hp_dice = fmt::format("{}d{}",
            mobile->hp_dice_num(),
            mobile->hp_dice_size());
        if (hp_bonus != 0) {
            hp_dice += fmt::format("{:+}", hp_bonus);
        }

        // Damage dice
        int dam_bonus = mobile->bare_hand_damage_dice_bonus();
        std::string damage_dice = fmt::format("{}d{}",
            mobile->bare_hand_damage_dice_num(),
            mobile->bare_hand_damage_dice_size());
        if (dam_bonus != 0) {
            damage_dice += fmt::format("{:+}", dam_bonus);
        }

        output << fmt::format("HP Dice: {}, Damage Dice: {}, Attack type: {}\n",
            hp_dice, damage_dice, mobile->damage_type());
        output << fmt::format("Mob Spec-Proc: None\n");
    }

    // Character flags
    if (mobile) {
        // Build list of set traits, behaviors, and professions
        std::vector<std::string> flag_names;

        // Traits (what mob IS)
        for (size_t i = 0; i < Mobile::MOB_TRAIT_CAPACITY; ++i) {
            auto trait = static_cast<MobTrait>(i);
            if (mobile->has_trait(trait)) {
                auto name = magic_enum::enum_name(trait);
                if (!name.empty()) {
                    flag_names.emplace_back(name);
                }
            }
        }

        // Behaviors (how mob ACTS)
        for (size_t i = 0; i < Mobile::MOB_BEHAVIOR_CAPACITY; ++i) {
            auto behavior = static_cast<MobBehavior>(i);
            if (mobile->has_behavior(behavior)) {
                auto name = magic_enum::enum_name(behavior);
                if (!name.empty()) {
                    flag_names.emplace_back(name);
                }
            }
        }

        // Professions (services mob provides)
        for (size_t i = 0; i < Mobile::MOB_PROFESSION_CAPACITY; ++i) {
            auto profession = static_cast<MobProfession>(i);
            if (mobile->has_profession(profession)) {
                auto name = magic_enum::enum_name(profession);
                if (!name.empty()) {
                    flag_names.emplace_back(name);
                }
            }
        }

        if (flag_names.empty()) {
            output << fmt::format("NPC flags: <None>\n");
        } else {
            std::string joined;
            for (size_t i = 0; i < flag_names.size(); ++i) {
                if (i > 0) joined += " ";
                joined += flag_names[i];
            }
            output << fmt::format("NPC flags: {}\n", joined);
        }
    } else {
        output << fmt::format("PLR: <None>\n");
        output << fmt::format("PRF: <None>\n");
        output << fmt::format("PRV: <None>\n");
    }

    // Weight and objects
    int worn_items = 0, worn_weight = 0;
    if (player) {
        auto equipped_items = player->equipment().get_all_equipped();
        for (const auto& item : equipped_items) {
            if (item) {
                worn_items++;
                worn_weight += item->weight();
            }
        }
    }

    output << fmt::format(
        "Max Carry: {} ({} weight); "
        "Carried: {} ({} weight); "
        "Worn: {} ({} weight)\n",
        20, max_carry_weight(),
        inventory().item_count(), current_carry_weight(),
        worn_items, worn_weight);

    // Show carried objects list
    const auto inventory_items = inventory().get_all_items();
    if (!inventory_items.empty()) {
        output << "Carrying:\n";
        for (const auto& item : inventory_items) {
            if (item) {
                output << fmt::format("  - {} [{}] (Weight: {})\n",
                    item->short_description(),
                    format_id(item->id()),
                    item->weight());
            }
        }
    } else {
        output << "Carrying: Nothing\n";
    }

    // Drunk condition (0=sober, higher=more drunk)
    if (stats_.drunk > 0) {
        std::string drunk_str;
        if (stats_.is_too_drunk()) drunk_str = "Very Drunk";
        else if (stats_.is_slurring()) drunk_str = "Tipsy";
        else drunk_str = "Slightly Buzzed";
        output << fmt::format("Intoxication: {} ({})\n", stats_.drunk, drunk_str);
    }

    // Followers and groups
    output << fmt::format("Consented: <none>, Master is: <none>, Followers are:\n");
    output << fmt::format("Group Master is: <none>, groupees are:\n");

    // Effects
    const auto& effects = active_effects_;
    if (effects.empty()) {
        output << "EFF: <None>\n";
    } else {
        output << "EFF: ";
        bool first = true;
        for (const auto& effect : effects) {
            if (!first) output << ", ";
            first = false;
            output << effect.name;
            if (effect.is_permanent()) {
                output << " (perm)";
            } else if (effect.duration_hours > 0) {
                output << fmt::format(" ({}h)", effect.duration_hours);
            }
        }
        output << "\n";
    }

    // Events
    output << "No events.\n";

    return output.str();
}
