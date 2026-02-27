#include <catch2/catch_test_macros.hpp>

#include "../../src/core/actor.hpp"
#include "../../src/core/object.hpp"
#include "../../src/core/player.hpp"
#include "../common/test_builders.hpp"

/**
 * Tests for Object type-specific properties.
 *
 * These verify that the property structs (DamageProfile, LightInfo, LiquidInfo,
 * ContainerInfo, portal settings, spell properties) work correctly through
 * setters/getters. These are the fields parsed from the DB "values" JSON column.
 */

// =============================================================================
// Weapon Damage Profile
// =============================================================================

TEST_CASE("ObjectProperties: DamageProfile", "[objects][properties][weapon]") {
    SECTION("Default damage profile is zeroed") {
        auto obj = ObjectBuilder().named("Plain Sword").as_weapon().build();
        auto profile = obj->damage_profile();
        REQUIRE(profile.base_damage == 0);
        REQUIRE(profile.dice_count == 0);
        REQUIRE(profile.dice_sides == 0);
        REQUIRE(profile.damage_bonus == 0);
    }

    SECTION("Set and retrieve damage profile") {
        auto obj = ObjectBuilder().named("Longsword").as_weapon().build();
        DamageProfile dmg;
        dmg.dice_count = 2;
        dmg.dice_sides = 6;
        dmg.damage_bonus = 3;
        obj->set_damage_profile(dmg);

        auto result = obj->damage_profile();
        REQUIRE(result.dice_count == 2);
        REQUIRE(result.dice_sides == 6);
        REQUIRE(result.damage_bonus == 3);
    }

    SECTION("Average damage calculation") {
        DamageProfile dmg;
        dmg.dice_count = 2;
        dmg.dice_sides = 6;
        dmg.damage_bonus = 3;
        // Average: 0 + 3 + (2 * 7) / 2.0 = 10.0
        REQUIRE(dmg.average_damage() == 10.0);
    }

    SECTION("Dice string formatting") {
        DamageProfile dmg;
        dmg.dice_count = 1;
        dmg.dice_sides = 8;
        dmg.damage_bonus = 2;
        REQUIRE(dmg.to_dice_string().find("1d8") != std::string::npos);
    }

    SECTION("Damage type set and get") {
        auto obj = ObjectBuilder().named("Fire Sword").as_weapon().build();
        obj->set_damage_type("Fire");
        REQUIRE(obj->damage_type() == "Fire");
    }
}

// =============================================================================
// Armor Properties
// =============================================================================

TEST_CASE("ObjectProperties: Armor", "[objects][properties][armor]") {
    SECTION("Default armor class is zero") {
        auto obj = ObjectBuilder().named("Leather Vest").as_armor().build();
        REQUIRE(obj->armor_class() == 0);
    }

    SECTION("Set and retrieve armor class") {
        auto obj = ObjectBuilder().named("Plate Mail").as_armor().build();
        obj->set_armor_class(5);
        REQUIRE(obj->armor_class() == 5);
    }
}

// =============================================================================
// Portal Properties
// =============================================================================

TEST_CASE("ObjectProperties: Portal", "[objects][properties][portal]") {
    SECTION("Default portal has no destination") {
        auto result = Object::create(EntityId(1, 1), "a portal", ObjectType::Portal);
        REQUIRE(result.has_value());
        auto obj = std::move(*result);
        // Portal destination should be invalid by default
        REQUIRE(!obj->has_portal_destination());
    }

    SECTION("Set and retrieve portal destination") {
        auto result = Object::create(EntityId(1, 2), "a magic portal", ObjectType::Portal);
        REQUIRE(result.has_value());
        auto obj = std::move(*result);

        obj->set_portal_destination(EntityId(50, 1));
        REQUIRE(obj->has_portal_destination());
        REQUIRE(obj->portal_destination() == EntityId(50, 1));
    }

    SECTION("Set and retrieve portal messages") {
        auto result = Object::create(EntityId(1, 3), "a swirling portal", ObjectType::Portal);
        REQUIRE(result.has_value());
        auto obj = std::move(*result);

        obj->set_portal_messages(1, 2, 3);
        REQUIRE(obj->portal_entry_msg() == 1);
        REQUIRE(obj->portal_char_msg() == 2);
        REQUIRE(obj->portal_exit_msg() == 3);
    }
}

// =============================================================================
// Light Properties
// =============================================================================

TEST_CASE("ObjectProperties: LightInfo", "[objects][properties][light]") {
    SECTION("Default light info") {
        LightInfo light;
        REQUIRE(light.duration == 0);
        REQUIRE(light.brightness == 1);
        REQUIRE(light.lit == false);
        REQUIRE(light.permanent == false);
    }

    SECTION("Set and retrieve light info") {
        auto result = Object::create(EntityId(1, 10), "a torch", ObjectType::Light);
        REQUIRE(result.has_value());
        auto obj = std::move(*result);

        LightInfo light;
        light.duration = 24;
        light.brightness = 3;
        light.lit = true;
        light.permanent = false;
        obj->set_light_info(light);

        auto stored = obj->light_info();
        REQUIRE(stored.duration == 24);
        REQUIRE(stored.brightness == 3);
        REQUIRE(stored.lit == true);
        REQUIRE(stored.permanent == false);
    }

    SECTION("Permanent lights are always lit") {
        LightInfo light;
        light.permanent = true;
        light.lit = true;
        light.duration = -1;
        REQUIRE(light.permanent == true);
        REQUIRE(light.lit == true);
    }
}

// =============================================================================
// Liquid/Drink Container Properties
// =============================================================================

TEST_CASE("ObjectProperties: LiquidInfo", "[objects][properties][drink]") {
    SECTION("Default liquid info") {
        LiquidInfo liquid;
        REQUIRE(liquid.liquid_type.empty());
        REQUIRE(liquid.capacity == 0);
        REQUIRE(liquid.remaining == 0);
        REQUIRE(liquid.effects.empty());
        REQUIRE(liquid.identified == false);
    }

    SECTION("Set and retrieve liquid info") {
        auto result = Object::create(EntityId(1, 20), "a water skin", ObjectType::Drinkcontainer);
        REQUIRE(result.has_value());
        auto obj = std::move(*result);

        LiquidInfo liquid;
        liquid.liquid_type = "WATER";
        liquid.capacity = 50;
        liquid.remaining = 25;
        liquid.effects = {1, 5, 12};
        liquid.identified = true;
        obj->set_liquid_info(liquid);

        auto stored = obj->liquid_info();
        REQUIRE(stored.liquid_type == "WATER");
        REQUIRE(stored.capacity == 50);
        REQUIRE(stored.remaining == 25);
        REQUIRE(stored.effects.size() == 3);
        REQUIRE(stored.effects[0] == 1);
        REQUIRE(stored.effects[1] == 5);
        REQUIRE(stored.effects[2] == 12);
        REQUIRE(stored.identified == true);
    }

    SECTION("Empty drink container") {
        LiquidInfo liquid;
        liquid.liquid_type = "ALE";
        liquid.capacity = 10;
        liquid.remaining = 0;
        REQUIRE(liquid.remaining == 0);
    }
}

// =============================================================================
// Container Properties
// =============================================================================

TEST_CASE("ObjectProperties: ContainerInfo", "[objects][properties][container]") {
    SECTION("Default container info") {
        ContainerInfo info;
        REQUIRE(info.capacity == 0);
        REQUIRE(info.weight_capacity == 0);
        REQUIRE(info.weight_reduction == 0);
        REQUIRE(info.closeable == false);
        REQUIRE(info.closed == false);
        REQUIRE(info.lockable == false);
        REQUIRE(info.locked == false);
    }

    SECTION("Set and retrieve container info") {
        auto obj = ObjectBuilder().named("a leather bag").as_container().build();

        ContainerInfo info;
        info.capacity = 20;
        info.weight_capacity = 200;
        info.weight_reduction = 50;
        info.closeable = true;
        info.closed = true;
        info.lockable = true;
        info.locked = false;
        info.key_id = EntityId(30, 5);
        obj->set_container_info(info);

        auto stored = obj->container_info();
        REQUIRE(stored.capacity == 20);
        REQUIRE(stored.weight_capacity == 200);
        REQUIRE(stored.weight_reduction == 50);
        REQUIRE(stored.closeable == true);
        REQUIRE(stored.closed == true);
        REQUIRE(stored.lockable == true);
        REQUIRE(stored.locked == false);
        REQUIRE(stored.key_id == EntityId(30, 5));
    }
}

// =============================================================================
// Spell/Magic Item Properties
// =============================================================================

TEST_CASE("ObjectProperties: Spell properties", "[objects][properties][magic]") {
    SECTION("Default spell properties are zero") {
        auto result = Object::create(EntityId(1, 30), "a scroll", ObjectType::Scroll);
        REQUIRE(result.has_value());
        auto obj = std::move(*result);

        REQUIRE(obj->spell_level() == 1); // Default spell level is 1 (clamped minimum)
        REQUIRE(obj->charges() == 0);
        REQUIRE(obj->max_charges() == 0);
    }

    SECTION("Set and retrieve spell level") {
        auto result = Object::create(EntityId(1, 31), "a potion of healing", ObjectType::Potion);
        REQUIRE(result.has_value());
        auto obj = std::move(*result);

        obj->set_spell_level(5);
        REQUIRE(obj->spell_level() == 5);
    }

    SECTION("Set and retrieve spell IDs") {
        auto result = Object::create(EntityId(1, 32), "a wand of fireballs", ObjectType::Wand);
        REQUIRE(result.has_value());
        auto obj = std::move(*result);

        obj->set_spell_id(0, 100);
        obj->set_spell_id(1, 200);
        obj->set_spell_id(2, 300);

        auto ids = obj->spell_ids();
        REQUIRE(ids[0] == 100);
        REQUIRE(ids[1] == 200);
        REQUIRE(ids[2] == 300);
    }

    SECTION("Set and retrieve charges") {
        auto result = Object::create(EntityId(1, 33), "a staff of power", ObjectType::Staff);
        REQUIRE(result.has_value());
        auto obj = std::move(*result);

        obj->set_charges(10);
        obj->set_max_charges(15);
        REQUIRE(obj->charges() == 10);
        REQUIRE(obj->max_charges() == 15);
    }

    SECTION("Max charges cannot be negative") {
        auto result = Object::create(EntityId(1, 34), "a broken wand", ObjectType::Wand);
        REQUIRE(result.has_value());
        auto obj = std::move(*result);

        obj->set_max_charges(-5);
        REQUIRE(obj->max_charges() == 0);
    }
}

// =============================================================================
// Extra Descriptions
// =============================================================================

TEST_CASE("ObjectProperties: Extra descriptions", "[objects][properties][descriptions]") {
    SECTION("Object starts with no extra descriptions") {
        auto obj = ObjectBuilder().named("a plain ring").build();
        REQUIRE(obj->get_all_extra_descriptions().empty());
    }

    SECTION("Add and retrieve extra description") {
        auto obj = ObjectBuilder().named("an engraved sword").as_weapon().build();

        ExtraDescription extra;
        extra.keywords = {"engraving", "runes"};
        extra.description = "Ancient runes are etched into the blade.";
        obj->add_extra_description(extra);

        auto all = obj->get_all_extra_descriptions();
        REQUIRE(all.size() == 1);
        REQUIRE(all[0].description == "Ancient runes are etched into the blade.");
        REQUIRE(all[0].keywords.size() == 2);
    }

    SECTION("Multiple extra descriptions") {
        auto obj = ObjectBuilder().named("a mysterious orb").build();

        ExtraDescription extra1;
        extra1.keywords = {"orb"};
        extra1.description = "The orb glows faintly.";
        obj->add_extra_description(extra1);

        ExtraDescription extra2;
        extra2.keywords = {"glow", "light"};
        extra2.description = "A soft blue light emanates from within.";
        obj->add_extra_description(extra2);

        REQUIRE(obj->get_all_extra_descriptions().size() == 2);
    }
}

// =============================================================================
// Effect Flags
// =============================================================================

TEST_CASE("ObjectProperties: Effect flags", "[objects][properties][effects]") {
    SECTION("Object starts with no effect flags") {
        auto obj = ObjectBuilder().named("a ring").build();
        REQUIRE(obj->effect_flags().empty());
    }

    SECTION("Set and check effect flags") {
        auto obj = ObjectBuilder().named("Ring of Protection").as_armor().build();
        obj->set_effect(EffectFlag::Sanctuary, true);
        REQUIRE(obj->has_effect(EffectFlag::Sanctuary));
        REQUIRE_FALSE(obj->has_effect(EffectFlag::Invisible));
    }

    SECTION("Remove effect flag") {
        auto obj = ObjectBuilder().named("Fading Ring").build();
        obj->set_effect(EffectFlag::DetectInvis, true);
        REQUIRE(obj->has_effect(EffectFlag::DetectInvis));

        obj->set_effect(EffectFlag::DetectInvis, false);
        REQUIRE_FALSE(obj->has_effect(EffectFlag::DetectInvis));
    }

    SECTION("Multiple effect flags") {
        auto obj = ObjectBuilder().named("Wizard's Robe").as_armor().build();
        obj->set_effect(EffectFlag::DetectMagic, true);
        obj->set_effect(EffectFlag::Infravision, true);
        obj->set_effect(EffectFlag::Waterbreath, true);

        REQUIRE(obj->has_effect(EffectFlag::DetectMagic));
        REQUIRE(obj->has_effect(EffectFlag::Infravision));
        REQUIRE(obj->has_effect(EffectFlag::Waterbreath));
        REQUIRE(obj->effect_flags().size() == 3);
    }
}

// =============================================================================
// Basic Object Properties
// =============================================================================

TEST_CASE("ObjectProperties: Weight, value, level", "[objects][properties][basic]") {
    SECTION("Weight") {
        auto obj = ObjectBuilder().named("a heavy shield").with_weight(25).build();
        REQUIRE(obj->weight() == 25);
    }

    SECTION("Value") {
        auto obj = ObjectBuilder().named("a gem").with_value(500).build();
        REQUIRE(obj->value() == 500);
    }

    SECTION("Level") {
        auto result = Object::create(EntityId(1, 50), "a level-gated sword", ObjectType::Weapon);
        REQUIRE(result.has_value());
        auto obj = std::move(*result);

        obj->set_level(10);
        REQUIRE(obj->level() == 10);
    }

    SECTION("Equip slot") {
        auto obj = ObjectBuilder().named("a helm").as_armor().build();
        obj->set_equip_slot(EquipSlot::Head);
        REQUIRE(obj->equip_slot() == EquipSlot::Head);
    }
}
