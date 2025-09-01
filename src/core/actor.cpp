/***************************************************************************
 *   File: s../core/actor.cpp                           Part of FieryMUD *
 *  Usage: Actor system implementation                                      *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.       *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 ***************************************************************************/

#include "actor.hpp"
#include "spell_system.hpp"
#include "../core/logging.hpp"
#include "../game/player_output.hpp"
#include "../world/room.hpp"
#include "../net/player_connection.hpp"

#include <algorithm>
#include <cmath>
#include <sstream>

// Stats Implementation

Result<void> Stats::validate() const {
    if (strength < 1 || strength > 25) {
        return std::unexpected(Errors::InvalidState("Strength must be between 1 and 25"));
    }
    if (dexterity < 1 || dexterity > 25) {
        return std::unexpected(Errors::InvalidState("Dexterity must be between 1 and 25"));
    }
    if (intelligence < 1 || intelligence > 25) {
        return std::unexpected(Errors::InvalidState("Intelligence must be between 1 and 25"));
    }
    if (wisdom < 1 || wisdom > 25) {
        return std::unexpected(Errors::InvalidState("Wisdom must be between 1 and 25"));
    }
    if (constitution < 1 || constitution > 25) {
        return std::unexpected(Errors::InvalidState("Constitution must be between 1 and 25"));
    }
    if (charisma < 1 || charisma > 25) {
        return std::unexpected(Errors::InvalidState("Charisma must be between 1 and 25"));
    }
    
    if (hit_points < 0 || hit_points > max_hit_points) {
        return std::unexpected(Errors::InvalidState("Hit points invalid"));
    }
    if (mana < 0 || mana > max_mana) {
        return std::unexpected(Errors::InvalidState("Mana invalid"));
    }
    if (movement < 0 || movement > max_movement) {
        return std::unexpected(Errors::InvalidState("Movement invalid"));
    }
    
    if (level < 1 || level > 100) {
        return std::unexpected(Errors::InvalidState("Level must be between 1 and 100"));
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
        {"movement", movement},
        {"max_movement", max_movement},
        {"armor_class", armor_class},
        {"hit_roll", hit_roll},
        {"damage_roll", damage_roll},
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
        if (json.contains("movement")) stats.movement = json["movement"].get<int>();
        if (json.contains("max_movement")) stats.max_movement = json["max_movement"].get<int>();
        
        if (json.contains("armor_class")) stats.armor_class = json["armor_class"].get<int>();
        if (json.contains("hit_roll")) stats.hit_roll = json["hit_roll"].get<int>();
        if (json.contains("damage_roll")) stats.damage_roll = json["damage_roll"].get<int>();
        
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
        if (item && item->matches_keyword(keyword)) {
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

std::shared_ptr<Object> Equipment::unequip_item(EquipSlot slot) {
    auto it = equipped_.find(slot);
    if (it != equipped_.end()) {
        auto item = it->second;
        equipped_.erase(it);
        return item;
    }
    return nullptr;
}

std::shared_ptr<Object> Equipment::unequip_item(EntityId item_id) {
    for (auto it = equipped_.begin(); it != equipped_.end(); ++it) {
        if (it->second && it->second->id() == item_id) {
            auto item = it->second;
            equipped_.erase(it);
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
    return weapon && weapon->has_flag(ObjectFlag::TwoHanded);
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
    if (has_flag(ActorFlag::Blind)) {
        return false;
    }
    
    if (object.has_flag(ObjectFlag::Invisible) && !has_flag(ActorFlag::Detect_Invis)) {
        return false;
    }
    
    return true;
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
    return 50 + Stats::attribute_modifier(stats_.strength) * 10;
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
    
    while (stats_.can_level_up() && stats_.level < 100) {
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
    
    // Increase movement
    int move_gain = 2 + Stats::attribute_modifier(stats_.constitution) / 2;
    stats_.max_movement += std::max(1, move_gain);
    stats_.movement = stats_.max_movement;
    
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

bool Actor::use_spell_slot(int circle) {
    return spell_slots_ && spell_slots_->use_slot(circle);
}

std::pair<int, int> Actor::get_spell_slot_info(int circle) const {
    if (!spell_slots_) {
        return {0, 0};
    }
    
    return spell_slots_->get_slot_info(circle);
}

void Actor::update_spell_slots() {
    if (spell_slots_) {
        spell_slots_->update_refresh();
    }
}

const SpellSlots& Actor::spell_slots() const {
    return *spell_slots_;
}

SpellSlots& Actor::spell_slots() {
    return *spell_slots_;
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
    
    if (level < 1 || level > 100) {
        return std::unexpected(Errors::InvalidArgument("level", "must be between 1 and 100"));
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
        if (json.contains("aggressive")) {
            mobile->set_aggressive(json["aggressive"].get<bool>());
        }
        
        if (json.contains("aggression_level")) {
            mobile->set_aggression_level(json["aggression_level"].get<int>());
        }
        
        // Parse alignment
        if (json.contains("alignment")) {
            int alignment = json["alignment"].get<int>();
            mobile->stats().alignment = alignment;
        }
        
        // Parse mob flags (array of strings or comma-separated string)
        if (json.contains("mob_flags")) {
            try {
                if (json["mob_flags"].is_array()) {
                    // Handle array format (modern JSON structure)
                    for (const auto& flag_json : json["mob_flags"]) {
                        std::string flag_name = flag_json.get<std::string>();
                        if (auto flag = ActorUtils::parse_flag(flag_name)) {
                            mobile->set_flag(flag.value(), true);
                        }
                    }
                } else {
                    // Handle comma-separated string format (legacy)
                    std::string flags_str = json["mob_flags"].get<std::string>();
                    if (!flags_str.empty()) {
                        std::istringstream iss(flags_str);
                        std::string flag_name;
                        while (std::getline(iss, flag_name, ',')) {
                            // Trim whitespace
                            flag_name.erase(0, flag_name.find_first_not_of(" \t"));
                            flag_name.erase(flag_name.find_last_not_of(" \t") + 1);
                            
                            if (auto flag = ActorUtils::parse_flag(flag_name)) {
                                mobile->set_flag(flag.value(), true);
                            }
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
                            // Trim whitespace
                            flag_name.erase(0, flag_name.find_first_not_of(" \t"));
                            flag_name.erase(flag_name.find_last_not_of(" \t") + 1);
                            
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
        if (json.contains("ac")) {
            stats.armor_class = json["ac"].get<int>();
        }
        if (json.contains("hit_roll")) {
            stats.hit_roll = json["hit_roll"].get<int>();
        }
        if (json.contains("damage_dice")) {
            // Handle damage dice - store bonus as damage_roll
            const auto& dmg_dice = json["damage_dice"];
            if (dmg_dice.contains("bonus")) {
                stats.damage_roll = dmg_dice["bonus"].get<int>();
            }
        }
        
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
                total_copper += parse_money_value(money["silver"]) * 10;
            }
            if (money.contains("gold")) {
                total_copper += parse_money_value(money["gold"]) * 100;
            }
            if (money.contains("platinum")) {
                total_copper += parse_money_value(money["platinum"]) * 1000;
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
    
    // Keep only the last 10 messages to avoid memory bloat
    if (received_messages_.size() > 10) {
        received_messages_.erase(received_messages_.begin());
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
    s.max_movement = ActorUtils::calculate_movement(level, s.constitution);
    s.movement = s.max_movement;
}

// Player Implementation

Player::Player(EntityId id, std::string_view name) 
    : Actor(id, name) {}

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
        player->set_description(base_entity->description());
        player->set_short_description(base_entity->short_description());
        
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
        
        if (json.contains("start_room")) {
            auto start_room_id = json["start_room"].get<uint64_t>();
            player->set_start_room(EntityId{start_room_id});
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
    if (output_) {
        output_->send_message(message);
    }
}

void Player::receive_message(std::string_view message) {
    send_message(message); // Players receive messages by sending them to output
}

nlohmann::json Player::get_vitals_gmcp() const {
    const auto& stats = this->stats();
    return {
        {"hp", stats.hit_points},
        {"max_hp", stats.max_hit_points},
        {"mv", stats.movement},
        {"max_mv", stats.max_movement}
        // Note: FieryMUD uses spell slots instead of mana
        // Spell slot information should be added separately when implemented
    };
}

nlohmann::json Player::get_status_gmcp() const {
    const auto& stats = this->stats();
    auto room = current_room();
    return {
        {"name", name()},
        {"level", stats.level},
        {"class", player_class_},
        {"race", race_},
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

nlohmann::json Player::to_json() const {
    nlohmann::json json = Actor::to_json();
    
    // Add Player-specific fields
    json["type"] = "Player";
    json["account"] = account_;
    json["god_level"] = god_level_;
    json["player_class"] = player_class_;
    json["race"] = race_;
    json["start_room"] = start_room_.value();
    
    return json;
}

// ActorUtils Implementation

namespace ActorUtils {
    long experience_for_level(int level) {
        if (level <= 1) return 0;
        
        // Use a progressive scale: level^2.5 * 1000
        return static_cast<long>(std::pow(level, 2.5) * 1000);
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
    
    int calculate_movement(int level, int constitution) {
        int base = 100 + (level * 2);
        int con_bonus = Stats::attribute_modifier(constitution) * 5;
        return std::max(50, base + con_bonus);
    }
    
    std::string format_stats(const Stats& stats) {
        return fmt::format("HP: {}/{}, Mana: {}/{}, Move: {}/{}, AC: {}, Level: {}", 
                          stats.hit_points, stats.max_hit_points,
                          stats.mana, stats.max_mana, 
                          stats.movement, stats.max_movement,
                          stats.armor_class, stats.level);
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
    
    // Vital character data - similar to legacy format
    output << fmt::format("{} '{}'  IDNum: [{:5}], In room [{}]\n", 
        player ? "PC" : (mobile ? "MOB" : "NPC"), 
        name(), 
        static_cast<uint32_t>(id().value()), 
        current_room() ? static_cast<uint32_t>(current_room()->id().value()) : 0);
    
    // Mobile-specific data
    if (mobile) {
        // For now, use the same name as alias since we don't have separate alias field
        output << fmt::format("Alias: {}\n", name());
    }
    
    // Descriptions
    if (mobile) {
        output << fmt::format("L-Des: {}\n", short_description().empty() ? "<None>" : short_description());
        output << fmt::format("Description: {}\n", mobile->description().empty() ? "<None>" : mobile->description());
    } else if (player) {
        output << fmt::format("Title: {}\n", player->title().empty() ? "<None>" : player->title());
    }
    
    // Character stats and abilities
    const auto& stats_ = stats();
    output << fmt::format("Race: {}, Race Align: N/A, ", "Unknown");
    output << fmt::format("Size: {}, Gender: {}\n", "Medium", "Neuter");
    output << fmt::format("Life force: Unknown, Composition: Unknown\n");
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
        auto play_hours = player->total_play_time().count() / 3600;
        
        output << fmt::format("Created: [{}], Last Logon: [{}], Played: [{}h]\n",
            creation, last_logon, play_hours);
        output << fmt::format("Age: [{}], Homeroom: [{}]", 
            20 + stats_.level, 
            static_cast<uint32_t>(player->start_room().value()));
        
        // TODO: Add clan/guild support
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
    
    // Health, mana, and movement
    output << fmt::format("HP: [{}/{}]  HP Gain: [{}] HP Regen Bonus: [{}]\n", 
        stats_.hit_points, stats_.max_hit_points, 1, 0);
    output << fmt::format("MV: [{}/{}]  MV Gain: [{}]\n", 
        stats_.movement, stats_.max_movement, 1);
    output << fmt::format("Focus: [{}]\n", stats_.mana);
    
    // Money (convert from gold to coins)
    long gold = stats_.gold;
    long platinum = gold / 1000; gold %= 1000;
    long silver = gold / 10; gold %= 10;
    long copper = gold;
    
    output << fmt::format(
        "Coins: [{}p / {}g / {}s / {}c], "
        "Bank: [0p / 0g / 0s / 0c]\n",
        platinum, gold, silver, copper);
    
    // Combat stats
    output << fmt::format("AC: [{}], Hitroll: [{:2}], Damroll: [{:2}], ", 
        stats_.armor_class, stats_.hit_roll, stats_.damage_roll);
    output << fmt::format("Saving throws: [0/0/0/0/0]\n");
    output << fmt::format("Perception: [   0], Hiddenness: [   0], Rage: [   0]\n");
    
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
        output << fmt::format("Mob Spec-Proc: None, NPC Bare Hand Dam: 1d4, Attack type: punch\n");
    }
    
    // Character flags
    if (mobile) {
        output << fmt::format("NPC flags: <None>\n");
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
                output << fmt::format("  - {} (ID: {}, Weight: {})\n", 
                    item->short_description(), 
                    static_cast<uint32_t>(item->id().value()), 
                    item->weight());
            }
        }
    } else {
        output << "Carrying: Nothing\n";
    }
    
    // Conditions (hunger, thirst, drunk)
    output << "Hunger: Off, Thirst: Off, Drunk: Off\n";
    
    // Followers and groups
    output << fmt::format("Consented: <none>, Master is: <none>, Followers are:\n");
    output << fmt::format("Group Master is: <none>, groupees are:\n");
    
    // Effects
    output << fmt::format("EFF: <None>\n");
    
    // Events
    output << "No events.\n";
    
    return output.str();
}