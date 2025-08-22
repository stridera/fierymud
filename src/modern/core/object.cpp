/***************************************************************************
 *   File: s../core/object.cpp                          Part of FieryMUD *
 *  Usage: Object hierarchy implementation                                  *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.       *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 ***************************************************************************/

#include "object.hpp"
#include "../core/logging.hpp"

#include <algorithm>
#include <unordered_map>

// DamageProfile Implementation

std::string DamageProfile::to_dice_string() const {
    if (dice_count == 0) {
        return std::to_string(base_damage + damage_bonus);
    }
    
    std::string result = fmt::format("{}d{}", dice_count, dice_sides);
    
    int total_bonus = base_damage + damage_bonus;
    if (total_bonus > 0) {
        result += fmt::format("+{}", total_bonus);
    } else if (total_bonus < 0) {
        result += std::to_string(total_bonus);
    }
    
    return result;
}

// Object Implementation

Object::Object(EntityId id, std::string_view name, ObjectType type) 
    : Entity(id, name), type_(type) {
    equip_slot_ = ObjectUtils::get_default_slot(type);
}

Result<std::unique_ptr<Object>> Object::create(EntityId id, std::string_view name, ObjectType type) {
    if (!id.is_valid()) {
        return std::unexpected(Errors::InvalidArgument("id", "must be valid"));
    }
    
    if (name.empty()) {
        return std::unexpected(Errors::InvalidArgument("name", "cannot be empty"));
    }
    
    auto object = std::unique_ptr<Object>(new Object(id, name, type));
    
    TRY(object->validate());
    
    return object;
}

Result<std::unique_ptr<Object>> Object::from_json(const nlohmann::json& json) {
    try {
        // First create base entity
        auto base_result = Entity::from_json(json);
        if (!base_result) {
            return std::unexpected(base_result.error());
        }
        
        auto base_entity = std::move(base_result.value());
        
        // Parse object-specific fields
        ObjectType type = ObjectType::Undefined;
        if (json.contains("object_type")) {
            if (auto parsed_type = ObjectUtils::parse_object_type(json["object_type"].get<std::string>())) {
                type = parsed_type.value();
            }
        }
        
        auto object = std::unique_ptr<Object>(new Object(base_entity->id(), base_entity->name(), type));
        
        // Copy base entity properties
        object->set_keywords(base_entity->keywords());
        object->set_description(base_entity->description());
        object->set_short_description(base_entity->short_description());
        
        // Parse object-specific properties
        if (json.contains("weight")) {
            object->set_weight(json["weight"].get<int>());
        }
        
        if (json.contains("value")) {
            object->set_value(json["value"].get<int>());
        }
        
        if (json.contains("condition")) {
            object->set_condition(json["condition"].get<int>());
        }
        
        if (json.contains("timer")) {
            object->set_timer(json["timer"].get<int>());
            object->has_timer_ = true;
        }
        
        if (json.contains("equip_slot")) {
            if (auto slot = ObjectUtils::parse_equip_slot(json["equip_slot"].get<std::string>())) {
                object->set_equip_slot(slot.value());
            }
        }
        
        if (json.contains("armor_class")) {
            object->set_armor_class(json["armor_class"].get<int>());
        }
        
        // Parse flags
        if (json.contains("flags") && json["flags"].is_array()) {
            for (const auto& flag_name : json["flags"]) {
                if (flag_name.is_string()) {
                    std::string flag_str = flag_name.get<std::string>();
                    if (auto flag = magic_enum::enum_cast<ObjectFlag>(flag_str)) {
                        object->set_flag(flag.value());
                    }
                }
            }
        }
        
        // Parse damage profile
        if (json.contains("damage_profile")) {
            const auto& dmg_json = json["damage_profile"];
            DamageProfile damage;
            
            if (dmg_json.contains("base_damage")) {
                damage.base_damage = dmg_json["base_damage"].get<int>();
            }
            if (dmg_json.contains("dice_count")) {
                damage.dice_count = dmg_json["dice_count"].get<int>();
            }
            if (dmg_json.contains("dice_sides")) {
                damage.dice_sides = dmg_json["dice_sides"].get<int>();
            }
            if (dmg_json.contains("damage_bonus")) {
                damage.damage_bonus = dmg_json["damage_bonus"].get<int>();
            }
            
            object->set_damage_profile(damage);
        }
        
        // Parse container info
        if (json.contains("container_info")) {
            const auto& cont_json = json["container_info"];
            ContainerInfo container;
            
            if (cont_json.contains("capacity")) {
                container.capacity = cont_json["capacity"].get<int>();
            }
            if (cont_json.contains("weight_capacity")) {
                container.weight_capacity = cont_json["weight_capacity"].get<int>();
            }
            if (cont_json.contains("closeable")) {
                container.closeable = cont_json["closeable"].get<bool>();
            }
            if (cont_json.contains("closed")) {
                container.closed = cont_json["closed"].get<bool>();
            }
            if (cont_json.contains("lockable")) {
                container.lockable = cont_json["lockable"].get<bool>();
            }
            if (cont_json.contains("locked")) {
                container.locked = cont_json["locked"].get<bool>();
            }
            if (cont_json.contains("key_id")) {
                container.key_id = EntityId{cont_json["key_id"].get<std::uint64_t>()};
            }
            
            object->set_container_info(container);
        }
        
        // Parse light info
        if (json.contains("light_info")) {
            const auto& light_json = json["light_info"];
            LightInfo light;
            
            if (light_json.contains("duration")) {
                light.duration = light_json["duration"].get<int>();
            }
            if (light_json.contains("brightness")) {
                light.brightness = light_json["brightness"].get<int>();
            }
            if (light_json.contains("lit")) {
                light.lit = light_json["lit"].get<bool>();
            }
            
            object->set_light_info(light);
        }
        
        TRY(object->validate());
        
        return object;
        
    } catch (const nlohmann::json::exception& e) {
        return std::unexpected(Errors::ParseError("Object JSON parsing error", e.what()));
    }
}

bool Object::has_flag(ObjectFlag flag) const {
    return flags_.contains(flag);
}

void Object::set_flag(ObjectFlag flag, bool value) {
    if (value) {
        flags_.insert(flag);
    } else {
        flags_.erase(flag);
    }
}

nlohmann::json Object::to_json() const {
    nlohmann::json json = Entity::to_json();
    
    json["object_type"] = std::string(magic_enum::enum_name(type_));
    json["weight"] = weight_;
    json["value"] = value_;
    json["condition"] = condition_;
    
    if (has_timer_) {
        json["timer"] = timer_;
    }
    
    json["equip_slot"] = std::string(magic_enum::enum_name(equip_slot_));
    json["armor_class"] = armor_class_;
    
    // Serialize flags
    std::vector<std::string> flag_names;
    for (ObjectFlag flag : flags_) {
        flag_names.emplace_back(magic_enum::enum_name(flag));
    }
    json["flags"] = flag_names;
    
    // Serialize damage profile if relevant
    if (is_weapon()) {
        json["damage_profile"] = {
            {"base_damage", damage_profile_.base_damage},
            {"dice_count", damage_profile_.dice_count},
            {"dice_sides", damage_profile_.dice_sides},
            {"damage_bonus", damage_profile_.damage_bonus}
        };
    }
    
    // Serialize container info if relevant
    if (is_container()) {
        json["container_info"] = {
            {"capacity", container_info_.capacity},
            {"weight_capacity", container_info_.weight_capacity},
            {"closeable", container_info_.closeable},
            {"closed", container_info_.closed},
            {"lockable", container_info_.lockable},
            {"locked", container_info_.locked},
            {"key_id", container_info_.key_id.value()}
        };
    }
    
    // Serialize light info if relevant
    if (is_light_source()) {
        json["light_info"] = {
            {"duration", light_info_.duration},
            {"brightness", light_info_.brightness},
            {"lit", light_info_.lit}
        };
    }
    
    return json;
}

Result<void> Object::validate() const {
    TRY(Entity::validate());
    
    if (type_ == ObjectType::Undefined) {
        return std::unexpected(Errors::InvalidState("Object type cannot be undefined"));
    }
    
    if (weight_ < 0) {
        return std::unexpected(Errors::InvalidState("Object weight cannot be negative"));
    }
    
    if (value_ < 0) {
        return std::unexpected(Errors::InvalidState("Object value cannot be negative"));
    }
    
    if (condition_ < 0 || condition_ > 100) {
        return std::unexpected(Errors::InvalidState("Object condition must be between 0 and 100"));
    }
    
    return Success();
}

std::string Object::display_name_with_condition(bool with_article) const {
    std::string base_name = display_name(with_article);
    std::string_view quality = quality_description();
    
    if (!quality.empty() && condition_ < 100) {
        return fmt::format("{} {}", quality, base_name);
    }
    
    return base_name;
}

std::string_view Object::quality_description() const {
    if (condition_ >= 90) return "";           // Perfect condition
    if (condition_ >= 80) return "slightly worn";
    if (condition_ >= 60) return "worn";
    if (condition_ >= 40) return "damaged";
    if (condition_ >= 20) return "badly damaged";
    if (condition_ > 0)   return "nearly broken";
    return "broken";
}

// Weapon Implementation

Weapon::Weapon(EntityId id, std::string_view name, ObjectType type) 
    : Object(id, name, type) {
    if (type != ObjectType::Weapon && type != ObjectType::Fireweapon) {
        set_type(ObjectType::Weapon);
    }
}

Result<std::unique_ptr<Weapon>> Weapon::create(EntityId id, std::string_view name, ObjectType weapon_type) {
    if (weapon_type != ObjectType::Weapon && weapon_type != ObjectType::Fireweapon) {
        return std::unexpected(Errors::InvalidArgument("weapon_type", "must be Weapon or Fireweapon"));
    }
    
    auto weapon = std::unique_ptr<Weapon>(new Weapon(id, name, weapon_type));
    
    TRY(weapon->validate());
    
    return weapon;
}

// Armor Implementation

Armor::Armor(EntityId id, std::string_view name, EquipSlot slot) 
    : Object(id, name, ObjectType::Armor) {
    set_equip_slot(slot);
}

Result<std::unique_ptr<Armor>> Armor::create(EntityId id, std::string_view name, EquipSlot slot) {
    if (slot == EquipSlot::None) {
        return std::unexpected(Errors::InvalidArgument("slot", "armor must have valid equip slot"));
    }
    
    auto armor = std::unique_ptr<Armor>(new Armor(id, name, slot));
    
    TRY(armor->validate());
    
    return armor;
}

// Container Implementation

Container::Container(EntityId id, std::string_view name, int capacity) 
    : Object(id, name, ObjectType::Container) {
    ContainerInfo info;
    info.capacity = capacity;
    info.weight_capacity = capacity * 10; // Default weight capacity
    set_container_info(info);
}

Result<std::unique_ptr<Container>> Container::create(EntityId id, std::string_view name, int capacity) {
    if (capacity <= 0) {
        return std::unexpected(Errors::InvalidArgument("capacity", "must be positive"));
    }
    
    auto container = std::unique_ptr<Container>(new Container(id, name, capacity));
    
    TRY(container->validate());
    
    return container;
}

bool Container::can_store_item(const Object& item) const {
    const auto& info = container_info();
    
    if (current_items_ >= info.capacity) {
        return false;
    }
    
    if (current_weight_ + item.weight() > info.weight_capacity) {
        return false;
    }
    
    return true;
}

// ObjectUtils Implementation

namespace ObjectUtils {
    std::string_view get_type_name(ObjectType type) {
        auto name = magic_enum::enum_name(type);
        return name.empty() ? "Unknown" : name;
    }
    
    std::optional<ObjectType> parse_object_type(std::string_view type_name) {
        return magic_enum::enum_cast<ObjectType>(type_name);
    }
    
    std::string_view get_slot_name(EquipSlot slot) {
        auto name = magic_enum::enum_name(slot);
        return name.empty() ? "None" : name;
    }
    
    std::optional<EquipSlot> parse_equip_slot(std::string_view slot_name) {
        return magic_enum::enum_cast<EquipSlot>(slot_name);
    }
    
    bool can_equip_in_slot(ObjectType type, EquipSlot slot) {
        static const std::unordered_map<ObjectType, std::unordered_set<EquipSlot>> valid_slots = {
            {ObjectType::Light, {EquipSlot::Light, EquipSlot::Hold}},
            {ObjectType::Weapon, {EquipSlot::Wield, EquipSlot::Wield2, EquipSlot::Hold}},
            {ObjectType::Fireweapon, {EquipSlot::Wield, EquipSlot::Wield2, EquipSlot::Hold}},
            {ObjectType::Armor, {EquipSlot::Body, EquipSlot::Head, EquipSlot::Legs, EquipSlot::Feet, 
                               EquipSlot::Hands, EquipSlot::Arms, EquipSlot::Shield}},
            {ObjectType::Worn, {EquipSlot::Finger_R, EquipSlot::Finger_L, EquipSlot::Neck1, EquipSlot::Neck2,
                               EquipSlot::About, EquipSlot::Waist, EquipSlot::Wrist_R, EquipSlot::Wrist_L,
                               EquipSlot::Hold, EquipSlot::Float, EquipSlot::Eye, EquipSlot::Ear, EquipSlot::Badge}},
            {ObjectType::Wings, {EquipSlot::Wings}},
            {ObjectType::Disguise, {EquipSlot::Disguise}}
        };
        
        auto it = valid_slots.find(type);
        if (it == valid_slots.end()) {
            return slot == EquipSlot::None;
        }
        
        return it->second.contains(slot);
    }
    
    EquipSlot get_default_slot(ObjectType type) {
        switch (type) {
            case ObjectType::Light: return EquipSlot::Light;
            case ObjectType::Weapon: return EquipSlot::Wield;
            case ObjectType::Fireweapon: return EquipSlot::Wield;
            case ObjectType::Armor: return EquipSlot::Body;
            case ObjectType::Wings: return EquipSlot::Wings;
            case ObjectType::Disguise: return EquipSlot::Disguise;
            default: return EquipSlot::None;
        }
    }
    
    int calculate_base_value(ObjectType type, int level) {
        static const std::unordered_map<ObjectType, int> base_values = {
            {ObjectType::Light, 5},
            {ObjectType::Scroll, 50},
            {ObjectType::Wand, 100},
            {ObjectType::Staff, 200},
            {ObjectType::Weapon, 30},
            {ObjectType::Fireweapon, 40},
            {ObjectType::Missile, 1},
            {ObjectType::Treasure, 100},
            {ObjectType::Armor, 50},
            {ObjectType::Potion, 25},
            {ObjectType::Worn, 20},
            {ObjectType::Other, 10},
            {ObjectType::Container, 15},
            {ObjectType::Food, 5},
            {ObjectType::Key, 1},
            {ObjectType::Money, 1}
        };
        
        auto it = base_values.find(type);
        int base = it != base_values.end() ? it->second : 10;
        
        return base * std::max(1, level);
    }
    
    std::string_view get_condition_color(int condition) {
        if (condition >= 90) return "\033[0;32m";     // Green (excellent)
        if (condition >= 70) return "\033[0;33m";     // Yellow (good)
        if (condition >= 50) return "\033[0;31m";     // Red (damaged)
        if (condition > 0)   return "\033[0;35m";     // Magenta (poor)
        return "\033[0;31m\033[5m";                   // Blinking red (broken)
    }
    
    std::string format_damage_profile(const DamageProfile& damage) {
        return damage.to_dice_string();
    }
}

// JSON Support Functions

void to_json(nlohmann::json& json, const Object& object) {
    json = object.to_json();
}

void from_json(const nlohmann::json& json, std::unique_ptr<Object>& object) {
    auto result = Object::from_json(json);
    if (result) {
        object = std::move(result.value());
    } else {
        throw std::runtime_error(fmt::format("Failed to create object from JSON: {}", result.error().message));
    }
}