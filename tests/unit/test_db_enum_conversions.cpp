#include <catch2/catch_test_macros.hpp>

#include "../../src/database/generated/db_mob.hpp"
#include "../../src/database/generated/db_object.hpp"

/**
 * Tests for database enum string<->enum round-trip conversions.
 *
 * These are generated functions that convert between SCREAMING_SNAKE_CASE
 * database strings and PascalCase C++ enums. A broken mapping silently
 * drops data on load.
 */

// =============================================================================
// Object Enums
// =============================================================================

TEST_CASE("DbEnums: ObjectType round-trip", "[database][enums][object]") {
    // Spot-check common types
    SECTION("Common types round-trip correctly") {
        auto weapon = db::object_type_from_db("WEAPON");
        REQUIRE(weapon.has_value());
        REQUIRE(db::object_type_to_db(*weapon) == "WEAPON");

        auto armor = db::object_type_from_db("ARMOR");
        REQUIRE(armor.has_value());
        REQUIRE(db::object_type_to_db(*armor) == "ARMOR");

        auto container = db::object_type_from_db("CONTAINER");
        REQUIRE(container.has_value());
        REQUIRE(db::object_type_to_db(*container) == "CONTAINER");

        auto potion = db::object_type_from_db("POTION");
        REQUIRE(potion.has_value());
        REQUIRE(db::object_type_to_db(*potion) == "POTION");

        auto portal = db::object_type_from_db("PORTAL");
        REQUIRE(portal.has_value());
        REQUIRE(db::object_type_to_db(*portal) == "PORTAL");

        auto light = db::object_type_from_db("LIGHT");
        REQUIRE(light.has_value());
        REQUIRE(db::object_type_to_db(*light) == "LIGHT");
    }

    SECTION("All ObjectType values have a DB string") {
        // Exhaustive: every enum value should produce a non-empty string
        for (int i = 0; i <= static_cast<int>(db::ObjectType::Disguise); ++i) {
            auto type = static_cast<db::ObjectType>(i);
            auto str = db::object_type_to_db(type);
            REQUIRE(!str.empty());

            // And the string should round-trip back to the same enum
            auto parsed = db::object_type_from_db(str);
            REQUIRE(parsed.has_value());
            REQUIRE(*parsed == type);
        }
    }

    SECTION("Invalid strings return nullopt") {
        REQUIRE_FALSE(db::object_type_from_db("INVALID_TYPE").has_value());
        REQUIRE_FALSE(db::object_type_from_db("").has_value());
        REQUIRE_FALSE(db::object_type_from_db("weapon").has_value()); // case-sensitive
    }
}

TEST_CASE("DbEnums: ObjectFlag round-trip", "[database][enums][object]") {
    SECTION("All ObjectFlag values round-trip") {
        for (int i = 0; i <= static_cast<int>(db::ObjectFlag::Soulbound); ++i) {
            auto flag = static_cast<db::ObjectFlag>(i);
            auto str = db::object_flag_to_db(flag);
            REQUIRE(!str.empty());

            auto parsed = db::object_flag_from_db(str);
            REQUIRE(parsed.has_value());
            REQUIRE(*parsed == flag);
        }
    }

    SECTION("Specific flag strings are correct") {
        REQUIRE(db::object_flag_to_db(db::ObjectFlag::Glow) == "GLOW");
        REQUIRE(db::object_flag_to_db(db::ObjectFlag::Hum) == "HUM");
        REQUIRE(db::object_flag_to_db(db::ObjectFlag::Float) == "FLOAT");
        REQUIRE(db::object_flag_to_db(db::ObjectFlag::Invisible) == "INVISIBLE");
        REQUIRE(db::object_flag_to_db(db::ObjectFlag::Magic) == "MAGIC");
    }
}

TEST_CASE("DbEnums: WearFlag round-trip", "[database][enums][object]") {
    SECTION("All WearFlag values round-trip") {
        for (int i = 0; i <= static_cast<int>(db::WearFlag::Disguise); ++i) {
            auto flag = static_cast<db::WearFlag>(i);
            auto str = db::wear_flag_to_db(flag);
            REQUIRE(!str.empty());

            auto parsed = db::wear_flag_from_db(str);
            REQUIRE(parsed.has_value());
            REQUIRE(*parsed == flag);
        }
    }

    SECTION("Common wear positions are correct") {
        REQUIRE(db::wear_flag_to_db(db::WearFlag::Head) == "HEAD");
        REQUIRE(db::wear_flag_to_db(db::WearFlag::Body) == "BODY");
        REQUIRE(db::wear_flag_to_db(db::WearFlag::Mainhand) == "MAINHAND");
        REQUIRE(db::wear_flag_to_db(db::WearFlag::Offhand) == "OFFHAND");
    }
}

// =============================================================================
// Mob Enums
// =============================================================================

TEST_CASE("DbEnums: MobRole round-trip", "[database][enums][mob]") {
    SECTION("All MobRole values round-trip") {
        for (int i = 0; i <= static_cast<int>(db::MobRole::RaidBoss); ++i) {
            auto role = static_cast<db::MobRole>(i);
            auto str = db::mob_role_to_db(role);
            REQUIRE(!str.empty());

            auto parsed = db::mob_role_from_db(str);
            REQUIRE(parsed.has_value());
            REQUIRE(*parsed == role);
        }
    }
}

TEST_CASE("DbEnums: MobTrait round-trip", "[database][enums][mob]") {
    SECTION("All MobTrait values round-trip") {
        for (int i = 0; i <= static_cast<int>(db::MobTrait::Pet); ++i) {
            auto trait = static_cast<db::MobTrait>(i);
            auto str = db::mob_trait_to_db(trait);
            REQUIRE(!str.empty());

            auto parsed = db::mob_trait_from_db(str);
            REQUIRE(parsed.has_value());
            REQUIRE(*parsed == trait);
        }
    }
}

TEST_CASE("DbEnums: MobBehavior round-trip", "[database][enums][mob]") {
    SECTION("All MobBehavior values round-trip") {
        for (int i = 0; i <= static_cast<int>(db::MobBehavior::NoKill); ++i) {
            auto behavior = static_cast<db::MobBehavior>(i);
            auto str = db::mob_behavior_to_db(behavior);
            REQUIRE(!str.empty());

            auto parsed = db::mob_behavior_from_db(str);
            REQUIRE(parsed.has_value());
            REQUIRE(*parsed == behavior);
        }
    }
}

TEST_CASE("DbEnums: MobProfession round-trip", "[database][enums][mob]") {
    SECTION("All MobProfession values round-trip") {
        for (int i = 0; i <= static_cast<int>(db::MobProfession::Trainer); ++i) {
            auto prof = static_cast<db::MobProfession>(i);
            auto str = db::mob_profession_to_db(prof);
            REQUIRE(!str.empty());

            auto parsed = db::mob_profession_from_db(str);
            REQUIRE(parsed.has_value());
            REQUIRE(*parsed == prof);
        }
    }
}

TEST_CASE("DbEnums: Position round-trip", "[database][enums][mob]") {
    SECTION("All Position values round-trip") {
        for (int i = 0; i <= static_cast<int>(db::Position::Standing); ++i) {
            auto pos = static_cast<db::Position>(i);
            auto str = db::position_to_db(pos);
            REQUIRE(!str.empty());

            auto parsed = db::position_from_db(str);
            REQUIRE(parsed.has_value());
            REQUIRE(*parsed == pos);
        }
    }
}

TEST_CASE("DbEnums: Stance round-trip", "[database][enums][mob]") {
    SECTION("All Stance values round-trip") {
        for (int i = 0; i <= static_cast<int>(db::Stance::Fighting); ++i) {
            auto stance = static_cast<db::Stance>(i);
            auto str = db::stance_to_db(stance);
            REQUIRE(!str.empty());

            auto parsed = db::stance_from_db(str);
            REQUIRE(parsed.has_value());
            REQUIRE(*parsed == stance);
        }
    }
}

TEST_CASE("DbEnums: Gender round-trip", "[database][enums][mob]") {
    SECTION("All Gender values round-trip") {
        for (int i = 0; i <= static_cast<int>(db::Gender::NonBinary); ++i) {
            auto gender = static_cast<db::Gender>(i);
            auto str = db::gender_to_db(gender);
            REQUIRE(!str.empty());

            auto parsed = db::gender_from_db(str);
            REQUIRE(parsed.has_value());
            REQUIRE(*parsed == gender);
        }
    }
}

TEST_CASE("DbEnums: Size round-trip", "[database][enums][mob]") {
    SECTION("All Size values round-trip") {
        for (int i = 0; i <= static_cast<int>(db::Size::Mountainous); ++i) {
            auto size = static_cast<db::Size>(i);
            auto str = db::size_to_db(size);
            REQUIRE(!str.empty());

            auto parsed = db::size_from_db(str);
            REQUIRE(parsed.has_value());
            REQUIRE(*parsed == size);
        }
    }
}

TEST_CASE("DbEnums: LifeForce round-trip", "[database][enums][mob]") {
    SECTION("All LifeForce values round-trip") {
        for (int i = 0; i <= static_cast<int>(db::LifeForce::Elemental); ++i) {
            auto lf = static_cast<db::LifeForce>(i);
            auto str = db::life_force_to_db(lf);
            REQUIRE(!str.empty());

            auto parsed = db::life_force_from_db(str);
            REQUIRE(parsed.has_value());
            REQUIRE(*parsed == lf);
        }
    }
}

TEST_CASE("DbEnums: Composition round-trip", "[database][enums][mob]") {
    SECTION("All Composition values round-trip") {
        for (int i = 0; i <= static_cast<int>(db::Composition::Plant); ++i) {
            auto comp = static_cast<db::Composition>(i);
            auto str = db::composition_to_db(comp);
            REQUIRE(!str.empty());

            auto parsed = db::composition_from_db(str);
            REQUIRE(parsed.has_value());
            REQUIRE(*parsed == comp);
        }
    }
}

TEST_CASE("DbEnums: DamageType round-trip", "[database][enums][mob]") {
    SECTION("All DamageType values round-trip") {
        for (int i = 0; i <= static_cast<int>(db::DamageType::Water); ++i) {
            auto dt = static_cast<db::DamageType>(i);
            auto str = db::damage_type_to_db(dt);
            REQUIRE(!str.empty());

            auto parsed = db::damage_type_from_db(str);
            REQUIRE(parsed.has_value());
            REQUIRE(*parsed == dt);
        }
    }

    SECTION("Combat-relevant damage types have correct strings") {
        REQUIRE(db::damage_type_to_db(db::DamageType::Slash) == "SLASH");
        REQUIRE(db::damage_type_to_db(db::DamageType::Pierce) == "PIERCE");
        REQUIRE(db::damage_type_to_db(db::DamageType::Fire) == "FIRE");
        REQUIRE(db::damage_type_to_db(db::DamageType::Cold) == "COLD");
        REQUIRE(db::damage_type_to_db(db::DamageType::Poison) == "POISON");
    }
}

// =============================================================================
// Alignment (shared between objects and mobs)
// =============================================================================

TEST_CASE("DbEnums: Alignment round-trip", "[database][enums]") {
    SECTION("All Alignment values round-trip") {
        for (int i = 0; i <= static_cast<int>(db::Alignment::Evil); ++i) {
            auto align = static_cast<db::Alignment>(i);
            auto str = db::alignment_to_db(align);
            REQUIRE(!str.empty());

            auto parsed = db::alignment_from_db(str);
            REQUIRE(parsed.has_value());
            REQUIRE(*parsed == align);
        }
    }
}
