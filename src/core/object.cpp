#include "object.hpp"
#include "../core/logging.hpp"

#include <algorithm>
#include <iostream>
#include <sstream>
#include <unordered_map>

// Object system constants
namespace {
    // Container defaults
    constexpr int DEFAULT_CONTAINER_CAPACITY = 10;      // Number of items
    constexpr int DEFAULT_WEIGHT_CAPACITY = 100;        // Weight units

    // Light defaults
    constexpr int DEFAULT_LIGHT_DURATION = 100;         // Time units
    constexpr int DEFAULT_LIGHT_BRIGHTNESS = 1;
    constexpr int BRIGHTNESS_CAPACITY_DIVISOR = 100;    // Scale capacity to brightness

    // Object condition constants
    constexpr int MIN_CONDITION = 0;
    constexpr int MAX_CONDITION = 100;
    constexpr int CONDITION_PERFECT = 90;
    constexpr int CONDITION_SLIGHTLY_WORN = 80;
    constexpr int CONDITION_WORN = 60;
    constexpr int CONDITION_DAMAGED = 40;
    constexpr int CONDITION_BADLY_DAMAGED = 20;

    // Spawn constraints
    constexpr int MOBILE_MAX_SPAWN_WEIGHT = 100;
}

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
            // Pass the actual type to preserve Liquid_Container vs Container distinction
            auto container_result = Container::create(id, name, DEFAULT_CONTAINER_CAPACITY, type);
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
                // Pass the actual type to preserve Liquid_Container vs Container distinction
                auto container_result = Container::create(base_entity->id(), base_entity->name(), container_capacity, type);
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
        
        if (json.contains("level")) {
            if (json["level"].is_string()) {
                object->set_level(std::stoi(json["level"].get<std::string>()));
            } else {
                object->set_level(json["level"].get<int>());
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
                    // Legacy format: "0" = no key, other values = key id
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
                default_container.capacity = DEFAULT_CONTAINER_CAPACITY;
                default_container.weight_capacity = DEFAULT_WEIGHT_CAPACITY;
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
                        light.brightness = std::max(1, capacity / BRIGHTNESS_CAPACITY_DIVISOR);
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
                default_light.duration = DEFAULT_LIGHT_DURATION;
                default_light.brightness = DEFAULT_LIGHT_BRIGHTNESS;
                default_light.lit = false;
                object->set_light_info(default_light);
            }
        }
        
        // Parse extra descriptions
        if (json.contains("extra_descriptions") && json["extra_descriptions"].is_array()) {
            for (const auto& extra_json : json["extra_descriptions"]) {
                if (extra_json.contains("keyword") && extra_json.contains("desc")) {
                    ExtraDescription extra;
                    
                    // Handle both single keyword string and array of keywords
                    if (extra_json["keyword"].is_string()) {
                        std::string keyword_str = extra_json["keyword"].get<std::string>();
                        // Split space-separated keywords into individual keywords
                        std::istringstream iss(keyword_str);
                        std::string word;
                        while (iss >> word) {
                            extra.keywords.push_back(word);
                        }
                    } else if (extra_json["keyword"].is_array()) {
                        for (const auto& kw : extra_json["keyword"]) {
                            if (kw.is_string()) {
                                extra.keywords.push_back(kw.get<std::string>());
                            }
                        }
                    }
                    
                    extra.description = extra_json["desc"].get<std::string>();
                    object->add_extra_description(extra);
                }
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

bool Object::has_effect(EffectFlag effect) const {
    return effect_flags_.contains(effect);
}

void Object::set_effect(EffectFlag effect, bool value) {
    if (value) {
        effect_flags_.insert(effect);
    } else {
        effect_flags_.erase(effect);
    }
}

nlohmann::json Object::to_json() const {
    nlohmann::json json = Entity::to_json();
    
    json["object_type"] = std::string(magic_enum::enum_name(type_));
    json["weight"] = weight_;
    json["value"] = value_;
    json["level"] = level_;
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
    
    // Serialize extra descriptions
    if (!extra_descriptions_.empty()) {
        nlohmann::json extra_array = nlohmann::json::array();
        for (const auto& extra : extra_descriptions_) {
            nlohmann::json extra_json;
            
            // Serialize keyword as single string if only one, array if multiple
            if (extra.keywords.size() == 1) {
                extra_json["keyword"] = extra.keywords[0];
            } else {
                extra_json["keyword"] = extra.keywords;
            }
            
            extra_json["desc"] = extra.description;
            extra_array.push_back(extra_json);
        }
        json["extra_descriptions"] = extra_array;
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
    
    if (condition_ < MIN_CONDITION || condition_ > MAX_CONDITION) {
        return std::unexpected(Errors::InvalidState(
            fmt::format("Object condition must be between {} and {}", MIN_CONDITION, MAX_CONDITION)));
    }
    
    return Success();
}

std::string Object::display_name_with_condition(bool with_article) const {
    std::string base_name = display_name(with_article);
    std::string_view quality = quality_description();

    if (!quality.empty() && condition_ < MAX_CONDITION) {
        return fmt::format("{} {}", quality, base_name);
    }

    return base_name;
}

std::string_view Object::quality_description() const {
    if (condition_ >= CONDITION_PERFECT) return "";                // Perfect condition
    if (condition_ >= CONDITION_SLIGHTLY_WORN) return "slightly worn";
    if (condition_ >= CONDITION_WORN) return "worn";
    if (condition_ >= CONDITION_DAMAGED) return "damaged";
    if (condition_ >= CONDITION_BADLY_DAMAGED) return "badly damaged";
    if (condition_ > MIN_CONDITION)   return "nearly broken";
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

Container::Container(EntityId id, std::string_view name, int capacity, ObjectType type)
    : Object(id, name, type) {
    ContainerInfo info;
    info.capacity = capacity;
    info.weight_capacity = capacity * 10; // Default weight capacity
    set_container_info(info);
}

Result<std::unique_ptr<Container>> Container::create(EntityId id, std::string_view name, int capacity, ObjectType type) {
    if (capacity <= 0) {
        return std::unexpected(Errors::InvalidArgument("capacity", "must be positive"));
    }

    auto container = std::unique_ptr<Container>(new Container(id, name, capacity, type));

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

void Container::add_item_force(std::shared_ptr<Object> item) {
    if (!item) {
        return;
    }

    // Bypass capacity/weight checks - used by zone resets
    contents_.push_back(item);
    current_items_ = contents_.size();
    current_weight_ += item->weight();
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
        // First try exact match with magic_enum
        if (auto slot = magic_enum::enum_cast<EquipSlot>(slot_name)) {
            return slot;
        }

        // Try case-insensitive match with magic_enum
        if (auto slot = magic_enum::enum_cast<EquipSlot>(slot_name, magic_enum::case_insensitive)) {
            return slot;
        }

        // Handle database-specific values that don't match enum names
        // Convert to uppercase for consistent comparison
        std::string upper_name(slot_name);
        for (auto& c : upper_name) {
            c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
        }

        // Map database values to EquipSlot
        static const std::unordered_map<std::string_view, EquipSlot> db_mappings = {
            // Standard mappings (uppercase version of enum names with underscores)
            {"FINGER_L", EquipSlot::Finger_L},
            {"FINGER_R", EquipSlot::Finger_R},
            {"NECK_1", EquipSlot::Neck1},
            {"NECK_2", EquipSlot::Neck2},
            {"WRIST_L", EquipSlot::Wrist_L},
            {"WRIST_R", EquipSlot::Wrist_R},

            // Legacy CircleMUD naming variations
            {"HOVER", EquipSlot::Float},          // Float slot
            {"EYES", EquipSlot::Eye},             // Eye slot (singular)
            {"LEAR", EquipSlot::Ear},             // Left ear → Ear slot
            {"REAR", EquipSlot::Ear},             // Right ear → Ear slot
            {"OBELT", EquipSlot::Waist},          // Outer belt → Waist
            {"HOLD2", EquipSlot::Hold},           // Second hold → Hold (same slot)
            {"TWO_HAND_WIELD", EquipSlot::Wield}, // Two-handed weapon → Wield

            // Common variations
            {"NONE", EquipSlot::None},
            {"", EquipSlot::None}
        };

        auto it = db_mappings.find(upper_name);
        if (it != db_mappings.end()) {
            return it->second;
        }

        // No match found
        return std::nullopt;
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
    // Show the display name and keywords for identification
    std::string keyword_list = EntityUtils::create_keyword_string(keywords());
    output << fmt::format("Name: '{}', Keywords: {}, Level: {}\n", 
        short_description().empty() ? "<None>" : short_description(), 
        keyword_list.empty() ? name() : keyword_list,
        level());

    output << fmt::format("ID: [    {}], Type: {}, SpecProc: None\n", 
        id(), magic_enum::enum_name(type()));

    output << fmt::format("L-Des: {}\n", 
        description().empty() ? "None" : description());
    
    // Action description (placeholder for future implementation)
    output << fmt::format("Action Description: None\n");
    
    // Wear flags
    std::string wear_flags = "<None>";
    if (is_wearable()) {
        wear_flags = "TAKE";
        // Add specific wear location flags based on equip_slot
        switch (equip_slot()) {
            case EquipSlot::Light: wear_flags += " LIGHT"; break;
            case EquipSlot::Finger_R: 
            case EquipSlot::Finger_L: wear_flags += " FINGER"; break;
            case EquipSlot::Neck1: 
            case EquipSlot::Neck2: wear_flags += " NECK"; break;
            case EquipSlot::Body: wear_flags += " BODY"; break;
            case EquipSlot::Head: wear_flags += " HEAD"; break;
            case EquipSlot::Legs: wear_flags += " LEGS"; break;
            case EquipSlot::Feet: wear_flags += " FEET"; break;
            case EquipSlot::Hands: wear_flags += " HANDS"; break;
            case EquipSlot::Arms: wear_flags += " ARMS"; break;
            case EquipSlot::Shield: wear_flags += " SHIELD"; break;
            case EquipSlot::About: wear_flags += " ABOUT"; break;
            case EquipSlot::Waist: wear_flags += " WAIST"; break;
            case EquipSlot::Wrist_R:
            case EquipSlot::Wrist_L: wear_flags += " WRIST"; break;
            case EquipSlot::Wield: wear_flags += " WIELD"; break;
            case EquipSlot::Hold: wear_flags += " HOLD"; break;
            case EquipSlot::Float: wear_flags += " FLOAT"; break;
            case EquipSlot::Wield2: wear_flags += " WIELD2"; break;
            case EquipSlot::Eye: wear_flags += " EYE"; break;
            case EquipSlot::Ear: wear_flags += " EAR"; break;
            case EquipSlot::Badge: wear_flags += " BADGE"; break;
            case EquipSlot::Focus: wear_flags += " FOCUS"; break;
            case EquipSlot::Throat: wear_flags += " THROAT"; break;
            case EquipSlot::Face: wear_flags += " FACE"; break;
            case EquipSlot::Wings: wear_flags += " WINGS"; break;
            case EquipSlot::Disguise: wear_flags += " DISGUISE"; break;
            default: break;
        }
    }
    output << fmt::format("Can be worn on: {}\n", wear_flags);
    
    // Extra flags
    std::string extra_flags = "<None>";
    if (!flags_.empty()) {
        std::vector<std::string> flag_names;
        for (const auto& flag : flags_) {
            flag_names.push_back(std::string(magic_enum::enum_name(flag)));
        }
        extra_flags = "";
        for (size_t i = 0; i < flag_names.size(); ++i) {
            if (i > 0) extra_flags += " ";
            extra_flags += flag_names[i];
        }
    }
    output << fmt::format("Extra flags   : {}\n", extra_flags);
    
    // Spell effects
    output << fmt::format("Spell Effects : <None>\n");
    
    // Weight, value, timer
    output << fmt::format("Weight: {:.2f}, Effective Weight: {:.2f}, Value: {}, Timer: {}, Decomp time: {}, Hiddenness: {}\n",
        static_cast<float>(weight()), 
        static_cast<float>(weight()), 
        value(), 
        timer(), 0, 0); // Decomp time and hiddenness not yet implemented
    
    // Location information
    output << fmt::format("In room: {}, In object: None, Carried by: Nobody, Worn by: Nobody\n",
        0); // Location tracking not yet implemented
    
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
        case ObjectType::Board:
            output << fmt::format("Board Number: {}\n", board_number_);
            break;
        default:
            output << fmt::format("Values: [0] [0] [0] [0] [0] [0] [0]\n");
            break;
    }
    
    // Container contents
    if (is_container()) {
        // Use dynamic_cast on this pointer instead of shared_from_this
        const auto* container = dynamic_cast<const Container*>(this);
        if (container && !container->is_empty()) {
            output << fmt::format("Contents ({} items):\n", container->contents_count());
            for (const auto& item : container->get_contents()) {
                if (item) {
                    output << fmt::format("  - {}\n", item->display_name());
                }
            }
        } else {
            output << "Contents: Empty\n";
        }
    }
    
    // Apply effects
    output << "Applies: None\n";
    
    // Extra descriptions
    const auto& extra_descs = get_all_extra_descriptions();
    if (!extra_descs.empty()) {
        output << "Extra descriptions:\n";
        for (const auto& desc : extra_descs) {
            // Use a descriptive label instead of showing keyword arrays to admins
            std::string label = desc.keywords.empty() ? "unknown" : 
                               (desc.keywords.size() == 1 ? desc.keywords[0] : 
                                fmt::format("{} (and {} others)", desc.keywords[0], desc.keywords.size() - 1));
            output << fmt::format("  {}: {}\n", label, desc.description);
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
    output << fmt::format("Contents weight: {}/{} lbs\n", contents_weight(), container_info().weight_capacity);
    
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