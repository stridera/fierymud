/***************************************************************************
 *   File: src/scripting/bindings/lua_object.cpp             Part of FieryMUD *
 *  Usage: Lua bindings for Object class                                    *
 ***************************************************************************/

#include "lua_object.hpp"
#include "../../core/object.hpp"

#define SOL_ALL_SAFETIES_ON 1
#include <sol/sol.hpp>

#include <fmt/format.h>
#include <spdlog/spdlog.h>

namespace FieryMUD {

void register_object_bindings(sol::state& lua) {
    // ObjectType enum
    lua.new_enum<ObjectType>("ObjectType",
        {
            {"Light", ObjectType::Light},
            {"Scroll", ObjectType::Scroll},
            {"Wand", ObjectType::Wand},
            {"Staff", ObjectType::Staff},
            {"Weapon", ObjectType::Weapon},
            {"Fireweapon", ObjectType::Fireweapon},
            {"Missile", ObjectType::Missile},
            {"Treasure", ObjectType::Treasure},
            {"Armor", ObjectType::Armor},
            {"Potion", ObjectType::Potion},
            {"Worn", ObjectType::Worn},
            {"Other", ObjectType::Other},
            {"Container", ObjectType::Container},
            {"Note", ObjectType::Note},
            {"LiquidContainer", ObjectType::Liquid_Container},
            {"Key", ObjectType::Key},
            {"Food", ObjectType::Food},
            {"Money", ObjectType::Money},
            {"Fountain", ObjectType::Fountain},
            {"Portal", ObjectType::Portal},
            {"Spellbook", ObjectType::Spellbook},
            {"Corpse", ObjectType::Corpse}
        }
    );

    // EquipSlot enum
    lua.new_enum<EquipSlot>("EquipSlot",
        {
            {"None", EquipSlot::None},
            {"Light", EquipSlot::Light},
            {"FingerR", EquipSlot::Finger_R},
            {"FingerL", EquipSlot::Finger_L},
            {"Neck1", EquipSlot::Neck1},
            {"Neck2", EquipSlot::Neck2},
            {"Body", EquipSlot::Body},
            {"Head", EquipSlot::Head},
            {"Legs", EquipSlot::Legs},
            {"Feet", EquipSlot::Feet},
            {"Hands", EquipSlot::Hands},
            {"Arms", EquipSlot::Arms},
            {"Shield", EquipSlot::Shield},
            {"About", EquipSlot::About},
            {"Waist", EquipSlot::Waist},
            {"WristR", EquipSlot::Wrist_R},
            {"WristL", EquipSlot::Wrist_L},
            {"Wield", EquipSlot::Wield},
            {"Hold", EquipSlot::Hold},
            {"Wield2", EquipSlot::Wield2}
        }
    );

    // ObjectFlag enum (subset for scripting)
    lua.new_enum<ObjectFlag>("ObjectFlag",
        {
            {"Glow", ObjectFlag::Glow},
            {"Hum", ObjectFlag::Hum},
            {"NoRent", ObjectFlag::NoRent},
            {"NoDonate", ObjectFlag::NoDonate},
            {"Invisible", ObjectFlag::Invisible},
            {"Magic", ObjectFlag::Magic},
            {"NoDrop", ObjectFlag::NoDrop},
            {"Bless", ObjectFlag::Bless},
            {"AntiGood", ObjectFlag::AntiGood},
            {"AntiEvil", ObjectFlag::AntiEvil},
            {"AntiNeutral", ObjectFlag::AntiNeutral},
            {"NoSell", ObjectFlag::NoSell},
            {"Cursed", ObjectFlag::Cursed},
            {"TwoHanded", ObjectFlag::TwoHanded},
            {"Poison", ObjectFlag::Poison}
        }
    );

    // Object class
    lua.new_usertype<Object>("Object",
        sol::no_constructor,

        // Basic properties
        "name", sol::property([](const Object& o) { return std::string(o.name()); }),
        "display_name", sol::property([](const Object& o) { return std::string(o.display_name()); }),
        "short_desc", sol::property([](const Object& o) { return std::string(o.short_description()); }),
        "shortdesc", sol::property([](const Object& o) { return std::string(o.short_description()); }),  // alias for DG script compat
        "id", sol::property([](const Object& o) -> std::string {
            auto id = o.id();
            return fmt::format("{}:{}", id.zone_id(), id.local_id());
        }),

        // Type information
        "type", sol::property(&Object::type),
        "equip_slot", sol::property(&Object::equip_slot),

        // Physical properties
        "weight", sol::property(&Object::weight),
        "base_weight", sol::property(&Object::base_weight),
        "value", sol::property(&Object::value),
        "level", sol::property(&Object::level),

        // Type checks
        "is_weapon", sol::property(&Object::is_weapon),
        "is_armor", sol::property(&Object::is_armor),
        "is_container", sol::property(&Object::is_container),
        "is_light_source", sol::property(&Object::is_light_source),
        "is_wearable", sol::property(&Object::is_wearable),
        "is_magic_item", sol::property(&Object::is_magic_item),
        "is_potion", sol::property(&Object::is_potion),
        "is_scroll", sol::property(&Object::is_scroll),
        "is_wand", sol::property(&Object::is_wand),
        "is_staff", sol::property(&Object::is_staff),

        // Type comparison
        "is_type", [](const Object& o, ObjectType type) -> bool {
            return o.type() == type;
        },

        // Flag checks
        "has_flag", [](const Object& o, ObjectFlag flag) -> bool {
            return o.has_flag(flag);
        },

        // Magic item properties
        "spell_level", sol::property(&Object::spell_level),

        // Weapon properties (safe access)
        "damage_dice_num", sol::property([](const Object& o) -> int {
            if (!o.is_weapon()) return 0;
            return o.damage_profile().dice_count;
        }),
        "damage_dice_size", sol::property([](const Object& o) -> int {
            if (!o.is_weapon()) return 0;
            return o.damage_profile().dice_sides;
        }),
        "damage_bonus", sol::property([](const Object& o) -> int {
            if (!o.is_weapon()) return 0;
            return o.damage_profile().damage_bonus;
        }),
        "average_damage", sol::property([](const Object& o) -> double {
            if (!o.is_weapon()) return 0.0;
            return o.damage_profile().average_damage();
        })
    );

    // Container class (extends Object)
    lua.new_usertype<Container>("Container",
        sol::no_constructor,
        sol::base_classes, sol::bases<Object>(),

        // Container-specific properties
        "capacity", sol::property([](const Container& c) -> int {
            return c.container_info().capacity;
        }),
        "item_count", sol::property([](const Container& c) -> int {
            return static_cast<int>(c.contents_count());
        }),
        "is_empty", sol::property([](const Container& c) -> bool {
            return c.is_empty();
        }),
        "is_full", sol::property([](const Container& c) -> bool {
            return static_cast<int>(c.contents_count()) >= c.container_info().capacity;
        }),
        "is_closed", sol::property([](const Container& c) -> bool {
            return c.container_info().closed;
        }),
        "is_locked", sol::property([](const Container& c) -> bool {
            return c.container_info().locked;
        }),

        // Container contents
        "items", sol::property([](Container& c)
            -> sol::as_table_t<std::vector<std::shared_ptr<Object>>> {
            auto contents = c.get_contents();
            std::vector<std::shared_ptr<Object>> result(contents.begin(), contents.end());
            return sol::as_table(result);
        }),

        // Find items
        "find_item", [](Container& c, const std::string& keyword) -> std::shared_ptr<Object> {
            auto items = c.find_items_by_keyword(keyword);
            return items.empty() ? nullptr : items[0];
        },

        "has_item", [](const Container& c, const std::string& keyword) -> bool {
            return !c.find_items_by_keyword(keyword).empty();
        }
    );

    spdlog::debug("Lua Object bindings registered");
}

} // namespace FieryMUD
