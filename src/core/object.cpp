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
#include <iostream>
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
    
    // Create appropriate subclass based on object type (same logic as from_json)
    std::unique_ptr<Object> object;
    switch (type) {
        case ObjectType::Container:
        case ObjectType::Liquid_Container: {
            // Default capacity for containers created via Object::create
            int container_capacity = 10;
            auto container_result = Container::create(id, name, container_capacity);
            if (!container_result.has_value()) {
                return std::unexpected(container_result.error());
            }
            object = std::move(container_result.value());
            break;
        }
        case ObjectType::Weapon:
        case ObjectType::Fireweapon: {
            auto weapon_result = Weapon::create(id, name, type);
            if (!weapon_result.has_value()) {
                return std::unexpected(weapon_result.error());
            }
            object = std::move(weapon_result.value());
            break;
        }
        case ObjectType::Armor: {
            auto armor_result = Armor::create(id, name);
            if (!armor_result.has_value()) {
                return std::unexpected(armor_result.error());
            }
            object = std::move(armor_result.value());
            break;
        }
        default: {
            // For all other types, create base Object
            object = std::unique_ptr<Object>(new Object(id, name, type));
            break;
        }
    }
    
    // Parse keywords from name (space-separated words)
    auto keywords = EntityUtils::parse_keyword_list(name);
    if (!keywords.empty()) {
        object->set_keywords(keywords);
    }
    
    TRY(object->validate());
    
    return object;
}

Result<std::unique_ptr<Object>> Object::from_json(const nlohmann::json& json) {
    try {
        // Parse base entity first
        auto base_result = Entity::from_json(json);
        if (!base_result.has_value()) {
            return std::unexpected(base_result.error());
        }
        
        auto base_entity = std::move(base_result.value());
        
        // Parse object-specific fields with legacy type mapping
        ObjectType type = ObjectType::Undefined;
        std::string type_str;
        if (json.contains("object_type")) {
            type_str = json["object_type"].get<std::string>();
        } else if (json.contains("type")) {
            type_str = json["type"].get<std::string>();
        }
        
        if (!type_str.empty()) {
            // Map legacy type names to modern enum names
            if (type_str == "NOTHING") {
                type = ObjectType::Other;  // Nothing -> Other (miscellaneous items) 
            } else if (type_str == "LIGHT") {
                type = ObjectType::Light;
            } else if (type_str == "SCROLL") {
                type = ObjectType::Scroll;
            } else if (type_str == "WAND") {
                type = ObjectType::Wand;
            } else if (type_str == "STAFF") {
                type = ObjectType::Staff;
            } else if (type_str == "WEAPON") {
                type = ObjectType::Weapon;
            } else if (type_str == "FIREWEAPON") {
                type = ObjectType::Fireweapon;
            } else if (type_str == "MISSILE") {
                type = ObjectType::Missile;
            } else if (type_str == "TREASURE") {
                type = ObjectType::Treasure;
            } else if (type_str == "ARMOR") {
                type = ObjectType::Armor;
            } else if (type_str == "POTION") {
                type = ObjectType::Potion;
            } else if (type_str == "WORN") {
                type = ObjectType::Worn;
            } else if (type_str == "OTHER") {
                type = ObjectType::Other;
            } else if (type_str == "TRASH") {
                type = ObjectType::Trash;
            } else if (type_str == "TRAP") {
                type = ObjectType::Trap;
            } else if (type_str == "CONTAINER") {
                type = ObjectType::Container;
            } else if (type_str == "NOTE") {
                type = ObjectType::Note;
            } else if (type_str == "DRINKCONTAINER" || type_str == "DRINKCON") {
                type = ObjectType::Liquid_Container;
            } else if (type_str == "KEY") {
                type = ObjectType::Key;
            } else if (type_str == "FOOD") {
                type = ObjectType::Food;
            } else if (type_str == "MONEY") {
                type = ObjectType::Money;
            } else if (type_str == "PEN") {
                type = ObjectType::Pen;
            } else if (type_str == "BOAT") {
                type = ObjectType::Boat;
            } else if (type_str == "FOUNTAIN") {
                type = ObjectType::Fountain;
            } else if (type_str == "PORTAL") {
                type = ObjectType::Portal;
            } else if (type_str == "ROPE") {
                type = ObjectType::Other;  // Map to Other as no specific Rope type in modern enum
            } else if (type_str == "SPELLBOOK") {
                type = ObjectType::Spellbook;
            } else if (type_str == "WALL") {
                type = ObjectType::Other;  // Map to Other as no specific Wall type in modern enum
            } else if (type_str == "TOUCHSTONE") {
                type = ObjectType::Other;  // Map to Other as no specific Touchstone type in modern enum
            } else if (type_str == "BOARD") {
                type = ObjectType::Board;
            } else if (type_str == "INSTRUMENT") {
                type = ObjectType::Other;  // Map to Other as no specific Instrument type in modern enum
            } else {
                // Try direct enum cast as fallback
                if (auto parsed_type = ObjectUtils::parse_object_type(type_str)) {
                    type = parsed_type.value();
                }
            }
        }
        
        // Pre-parse capacity for containers to create with correct size
        int container_capacity = 10; // Default capacity
        if (type == ObjectType::Container || type == ObjectType::Liquid_Container) {
            // Check for modern container_info format
            if (json.contains("container_info") && json["container_info"].contains("capacity")) {
                container_capacity = json["container_info"]["capacity"].get<int>();
            }
            // Check for legacy values format 
            else if (json.contains("values") && json["values"].contains("Capacity")) {
                try {
                    const std::string cap_str = json["values"]["Capacity"].get<std::string>();
                    if (!cap_str.empty() && cap_str != "0") {
                        container_capacity = std::stoi(cap_str);
                    }
                } catch (const std::exception&) {
                    // Use default if parsing fails
                }
            }
        }
        
        // Create appropriate subclass based on object type
        std::unique_ptr<Object> object;
        switch (type) {
            case ObjectType::Container:
            case ObjectType::Liquid_Container: {
                auto container_result = Container::create(base_entity->id(), base_entity->name(), container_capacity);
                if (!container_result.has_value()) {
                    return std::unexpected(container_result.error());
                }
                object = std::move(container_result.value());
                break;
            }
            case ObjectType::Weapon:
            case ObjectType::Fireweapon: {
                auto weapon_result = Weapon::create(base_entity->id(), base_entity->name(), type);
                if (!weapon_result.has_value()) {
                    return std::unexpected(weapon_result.error());
                }
                object = std::move(weapon_result.value());
                break;
            }
            case ObjectType::Armor: {
                auto armor_result = Armor::create(base_entity->id(), base_entity->name());
                if (!armor_result.has_value()) {
                    return std::unexpected(armor_result.error());
                }
                object = std::move(armor_result.value());
                break;
            }
            default: {
                object = std::unique_ptr<Object>(new Object(base_entity->id(), base_entity->name(), type));
                break;
            }
        }
        
        // Copy all base entity properties
        object->set_keywords(base_entity->keywords());
        object->set_description(base_entity->description());
        object->set_short_description(base_entity->short_description());
        
        // Parse object-specific properties with field mapping
        if (json.contains("weight")) {
            if (json["weight"].is_string()) {
                object->set_weight(static_cast<int>(std::stof(json["weight"].get<std::string>())));
            } else {
                object->set_weight(json["weight"].get<int>());
            }
        }
        
        if (json.contains("value")) {
            object->set_value(json["value"].get<int>());
        } else if (json.contains("cost")) {
            if (json["cost"].is_string()) {
                object->set_value(std::stoi(json["cost"].get<std::string>()));
            } else {
                object->set_value(json["cost"].get<int>());
            }
        }
        
        if (json.contains("condition")) {
            object->set_condition(json["condition"].get<int>());
        }
        
        if (json.contains("timer")) {
            int timer_value;
            if (json["timer"].is_string()) {
                timer_value = std::stoi(json["timer"].get<std::string>());
            } else {
                timer_value = json["timer"].get<int>();
            }
            object->set_timer(timer_value);
            object->has_timer_ = (timer_value > 0);
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
        // Parse legacy container info from "values" section for CONTAINER type objects
        else if (type == ObjectType::Container && json.contains("values")) {
            const auto& values_json = json["values"];
            ContainerInfo container;
            
            // Parse legacy container values with error handling
            try {
                if (values_json.contains("Capacity")) {
                    // Legacy format stores capacity as string
                    std::string capacity_str = values_json["Capacity"].get<std::string>();
                    if (!capacity_str.empty()) {
                        container.capacity = std::stoi(capacity_str);
                        container.weight_capacity = container.capacity * 10;  // Default weight capacity
                    }
                }
                if (values_json.contains("Key")) {
                    // Legacy format: "0" = no key, other values = key vnum
                    std::string key_str = values_json["Key"].get<std::string>();
                    if (!key_str.empty() && key_str != "0") {
                        container.key_id = EntityId{static_cast<std::uint64_t>(std::stoi(key_str))};
                        container.lockable = true;
                    }
                }
                if (values_json.contains("Flags")) {
                    // Legacy container flags could include closeable/lockable info
                    std::string flags_str = values_json["Flags"].get<std::string>();
                    // For now, assume basic container functionality
                    container.closeable = true;  // Most containers can be closed
                    container.closed = false;    // Start open
                }
                
                object->set_container_info(container);
            } catch (const std::exception& e) {
                // If parsing legacy values fails, use defaults but continue loading
                ContainerInfo default_container;
                default_container.capacity = 10;        // Default capacity
                default_container.weight_capacity = 100; // Default weight capacity
                default_container.closeable = true;
                default_container.closed = false;
                object->set_container_info(default_container);
            }
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
        // Parse legacy light info from "values" section for LIGHT type objects
        else if (type == ObjectType::Light && json.contains("values")) {
            const auto& values_json = json["values"];
            LightInfo light;
            
            // Parse legacy light values with error handling
            try {
                if (values_json.contains("Remaining")) {
                    // Legacy format stores remaining duration as string
                    std::string remaining_str = values_json["Remaining"].get<std::string>();
                    if (!remaining_str.empty()) {
                        light.duration = std::stoi(remaining_str);
                    }
                }
                if (values_json.contains("Capacity")) {
                    // Use capacity as brightness indicator (higher capacity = brighter)
                    std::string capacity_str = values_json["Capacity"].get<std::string>();
                    if (!capacity_str.empty()) {
                        int capacity = std::stoi(capacity_str);
                        light.brightness = std::max(1, capacity / 100);  // Scale down capacity to brightness
                    }
                }
                if (values_json.contains("Is_Lit:")) {
                    // Legacy format: "0" = not lit, "1" = lit
                    std::string lit_str = values_json["Is_Lit:"].get<std::string>();
                    light.lit = (!lit_str.empty() && lit_str != "0");
                }
                
                object->set_light_info(light);
            } catch (const std::exception& e) {
                // If parsing legacy values fails, use defaults but continue loading
                // This prevents server crashes from malformed legacy data
                LightInfo default_light;
                default_light.duration = 100;  // Default duration for legacy lights
                default_light.brightness = 1;
                default_light.lit = false;
                object->set_light_info(default_light);
            }
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

Result<void> Container::add_item(std::shared_ptr<Object> item) {
    if (!item) {
        return std::unexpected(Errors::InvalidArgument("item", "cannot be null"));
    }
    
    if (!can_store_item(*item)) {
        return std::unexpected(Errors::InvalidState("container is full"));
    }
    
    contents_.push_back(item);
    current_items_ = contents_.size();
    
    // Recalculate current weight
    current_weight_ = 0;
    for (const auto& obj : contents_) {
        if (obj) {
            current_weight_ += obj->weight();
        }
    }
    
    return Success();
}

std::shared_ptr<Object> Container::remove_item(EntityId item_id) {
    for (auto it = contents_.begin(); it != contents_.end(); ++it) {
        if (*it && (*it)->id() == item_id) {
            auto item = *it;
            contents_.erase(it);
            current_items_ = contents_.size();
            
            // Recalculate current weight
            current_weight_ = 0;
            for (const auto& obj : contents_) {
                if (obj) {
                    current_weight_ += obj->weight();
                }
            }
            
            return item;
        }
    }
    return nullptr;
}

bool Container::remove_item(const std::shared_ptr<Object>& item) {
    if (!item) return false;
    
    auto it = std::find(contents_.begin(), contents_.end(), item);
    if (it != contents_.end()) {
        contents_.erase(it);
        current_items_ = contents_.size();
        
        // Recalculate current weight
        current_weight_ = 0;
        for (const auto& obj : contents_) {
            if (obj) {
                current_weight_ += obj->weight();
            }
        }
        
        return true;
    }
    return false;
}

std::shared_ptr<Object> Container::find_item(EntityId item_id) const {
    for (const auto& item : contents_) {
        if (item && item->id() == item_id) {
            return item;
        }
    }
    return nullptr;
}

std::vector<std::shared_ptr<Object>> Container::find_items_by_keyword(std::string_view keyword) const {
    std::vector<std::shared_ptr<Object>> results;
    for (const auto& item : contents_) {
        if (item && item->matches_keyword(keyword)) {
            results.push_back(item);
        }
    }
    return results;
}

std::span<const std::shared_ptr<Object>> Container::get_contents() const {
    return contents_;
}

// Object extra description implementation

void Object::add_extra_description(const ExtraDescription& extra_desc) {
    extra_descriptions_.push_back(extra_desc);
}

std::optional<std::string_view> Object::get_extra_description(std::string_view keyword) const {
    for (const auto& extra : extra_descriptions_) {
        if (extra.matches_keyword(keyword)) {
            return extra.description;
        }
    }
    return std::nullopt;
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
    if (result.has_value()) {
        object = std::move(result.value());
    } else {
        throw std::runtime_error("Failed to parse object from JSON");
    }
}

std::string Object::get_stat_info() const {
    std::ostringstream output;
    
    // Basic object information - similar to legacy format
    output << fmt::format("Name: '{}', Aliases: {}, Level: {}\n", 
        short_description().empty() ? "<None>" : short_description(), 
        name(), 
        1); // TODO: Add object level support
    
    output << fmt::format("VNum: [    0], RNum: [    0], Type: {}, SpecProc: None\n", 
        magic_enum::enum_name(type()));
    
    output << fmt::format("L-Des: {}\n", 
        description().empty() ? "None" : description());
    
    // TODO: Add action description support
    
    // Wear flags
    std::string wear_flags = "<None>";
    if (is_wearable()) {
        wear_flags = "TAKE";
        // TODO: Add specific wear location flags based on equip_slot
        switch (equip_slot()) {
            case EquipSlot::Head: wear_flags += " HEAD"; break;
            case EquipSlot::Body: wear_flags += " BODY"; break;
            case EquipSlot::Arms: wear_flags += " ARMS"; break;
            case EquipSlot::Hands: wear_flags += " HANDS"; break;
            case EquipSlot::Legs: wear_flags += " LEGS"; break;
            case EquipSlot::Feet: wear_flags += " FEET"; break;
            case EquipSlot::Wield: wear_flags += " WIELD"; break;
            case EquipSlot::Shield: wear_flags += " SHIELD"; break;
            case EquipSlot::Neck1: wear_flags += " NECK"; break;
            case EquipSlot::Finger_R: wear_flags += " FINGER"; break;
            default: break;
        }
    }
    output << fmt::format("Can be worn on: {}\n", wear_flags);
    
    // Extra flags
    std::string extra_flags = "<None>";
    // TODO: Add object flags support based on flags_ set
    output << fmt::format("Extra flags   : {}\n", extra_flags);
    
    // Spell effects
    output << fmt::format("Spell Effects : <None>\n");
    
    // Weight, value, timer
    output << fmt::format("Weight: {:.2f}, Effective Weight: {:.2f}, Value: {}, Timer: {}, Decomp time: {}, Hiddenness: {}\n",
        static_cast<float>(weight()), 
        static_cast<float>(weight()), 
        value(), 
        timer(), 0, 0); // TODO: Add decomp, hiddenness support
    
    // Location information
    output << fmt::format("In room: {}, In object: None, Carried by: Nobody, Worn by: Nobody\n",
        0); // TODO: Add location tracking
    
    // Type-specific information
    switch (type()) {
        case ObjectType::Weapon:
        case ObjectType::Fireweapon: {
            const auto& dmg = damage_profile();
            output << fmt::format("Todam: {}d{} (avg {:.1f}), Message type: 0, 'punch'\n",
                dmg.dice_count > 0 ? dmg.dice_count : 1,
                dmg.dice_sides > 0 ? dmg.dice_sides : 4,
                dmg.dice_count > 0 && dmg.dice_sides > 0 ? 
                    static_cast<float>(dmg.dice_count * (dmg.dice_sides + 1)) / 2.0f : 2.5f);
            break;
        }
        case ObjectType::Armor:
            output << fmt::format("AC-apply: [{}]\n", armor_class());
            break;
        case ObjectType::Container:
        case ObjectType::Liquid_Container:
            if (is_container()) {
                const auto& info = container_info();
                output << fmt::format(
                    "Weight capacity: {}, Lock Type: {}, Key Num: {}, Weight Reduction: {}%, Corpse: {}\n",
                    info.weight_capacity, 
                    info.lockable ? (info.locked ? "LOCKED" : "CLOSEABLE") : "NONE",
                    info.key_id.is_valid() ? static_cast<uint32_t>(info.key_id.value()) : 0,
                    0, "No");
            } else {
                output << fmt::format("Weight capacity: 0, Lock Type: NONE, Key Num: 0, Weight Reduction: 0%, Corpse: No\n");
            }
            break;
        case ObjectType::Light:
            if (is_light_source()) {
                const auto& light = light_info();
                if (light.duration == -1) {
                    output << fmt::format("Hours left: Infinite\n");
                } else {
                    output << fmt::format("Hours left: [{}]  Initial hours: [{}]\n", 
                        light.duration, light.brightness);
                }
            } else {
                output << fmt::format("Hours left: [0]  Initial hours: [0]\n");
            }
            break;
        case ObjectType::Food:
            output << fmt::format("Makes full: 1, Poisoned: No\n");
            break;
        case ObjectType::Money:
            output << fmt::format("Coins: 0p 1g 0s 0c\n");
            break;
        case ObjectType::Scroll:
        case ObjectType::Potion:
            output << fmt::format("Spells: (Level 1) <none>, <none>, <none>\n");
            break;
        case ObjectType::Wand:
        case ObjectType::Staff:
            output << fmt::format("Spell: <none> at level 1, 0 (of 0) charges remaining\n");
            break;
        case ObjectType::Fountain:
            output << fmt::format("Capacity: 0, Contains: 0, Poisoned: No, Liquid: water\n");
            break;
        case ObjectType::Note:
            output << fmt::format("Tongue: 0\n");
            break;
        case ObjectType::Portal:
            output << fmt::format("To room: 0\n");
            break;
        default:
            output << fmt::format("Values: [0] [0] [0] [0] [0] [0] [0]\n");
            break;
    }
    
    // Container contents
    if (is_container()) {
        // TODO: Add container contents listing when we have contained objects
    }
    
    // Apply effects
    output << "Applies: None\n";
    
    // Extra descriptions
    const auto& extra_descs = get_all_extra_descriptions();
    if (!extra_descs.empty()) {
        output << "Extra descriptions:\n";
        for (const auto& desc : extra_descs) {
            // Format keywords as comma-separated string
            std::string keywords_str;
            for (size_t i = 0; i < desc.keywords.size(); ++i) {
                if (i > 0) keywords_str += ", ";
                keywords_str += desc.keywords[i];
            }
            output << fmt::format("  {}: {}\n", keywords_str, desc.description);
        }
    }
    
    // Events
    output << "No events.\n";
    
    return output.str();
}

// Specialized stat info implementations for derived classes

std::string Container::get_stat_info() const {
    std::ostringstream output;
    
    // Start with base object information
    output << Object::get_stat_info();
    
    // Add container-specific details
    output << "\n=== Container Details ===\n";
    output << fmt::format("Current contents: {}/{} items\n", contents_count(), container_info().capacity);
    output << fmt::format("Current weight: {}/{} lbs\n", current_weight(), container_info().weight_capacity);
    
    if (!is_empty()) {
        output << "Contains:\n";
        const auto contents = get_contents();
        for (const auto& item : contents) {
            if (item) {
                output << fmt::format("  {} ({})\n", 
                    item->short_description(), 
                    item->display_name_with_condition());
            }
        }
    } else {
        output << "Container is empty.\n";
    }
    
    return output.str();
}

std::string Weapon::get_stat_info() const {
    std::ostringstream output;
    
    // Start with base object information
    output << Object::get_stat_info();
    
    // Add weapon-specific details
    output << "\n=== Weapon Details ===\n";
    output << fmt::format("Weapon type: {}\n", is_ranged() ? "Ranged" : "Melee");
    output << fmt::format("Reach: {} feet\n", reach());
    output << fmt::format("Speed: {} (lower is faster)\n", speed());
    
    const auto& dmg = damage_profile();
    if (dmg.dice_count > 0 && dmg.dice_sides > 0) {
        output << fmt::format("Damage: {}d{}", dmg.dice_count, dmg.dice_sides);
        if (dmg.damage_bonus != 0) {
            output << fmt::format("{:+d}", dmg.damage_bonus);
        }
        output << fmt::format(" (avg: {:.1f})\n", dmg.average_damage());
    }
    
    return output.str();
}

std::string Armor::get_stat_info() const {
    std::ostringstream output;
    
    // Start with base object information
    output << Object::get_stat_info();
    
    // Add armor-specific details
    output << "\n=== Armor Details ===\n";
    output << fmt::format("Material: {}\n", material());
    output << fmt::format("Armor Class bonus: {}\n", armor_class());
    output << fmt::format("Equip slot: {}\n", ObjectUtils::get_slot_name(equip_slot()));
    output << fmt::format("Condition: {}% ({})\n", condition(), quality_description());
    
    return output.str();
}