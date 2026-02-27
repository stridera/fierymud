#include <utility>

#include <catch2/catch_test_macros.hpp>

#include "../../src/core/actor.hpp"
#include "../../src/core/object.hpp"
#include "../../src/core/player.hpp"
#include "../../src/database/db_parsing_utils.hpp"
#include "../../src/database/generated/db_object.hpp"
#include "../common/test_builders.hpp"

// =============================================================================
// DB-to-Game Enum Conversion Tests
// =============================================================================

TEST_CASE("ObjectFlags: DB-to-game enum mapping is correct", "[database][flags][unit]") {
    SECTION("Every mapped DB flag converts to the correct game flag") {
        // These are the flags that existed in the broken static_cast version.
        // The old code did: return static_cast<ObjectFlag>(std::to_underlying(f))
        // which silently mapped to WRONG values because the enums have different numbering.

        auto glow = DbParsingUtils::object_flag_to_game(db::ObjectFlag::Glow);
        REQUIRE(glow.has_value());
        REQUIRE(*glow == ObjectFlag::Glow);

        auto hum = DbParsingUtils::object_flag_to_game(db::ObjectFlag::Hum);
        REQUIRE(hum.has_value());
        REQUIRE(*hum == ObjectFlag::Hum);

        auto invisible = DbParsingUtils::object_flag_to_game(db::ObjectFlag::Invisible);
        REQUIRE(invisible.has_value());
        REQUIRE(*invisible == ObjectFlag::Invisible);

        auto magic = DbParsingUtils::object_flag_to_game(db::ObjectFlag::Magic);
        REQUIRE(magic.has_value());
        REQUIRE(*magic == ObjectFlag::Magic);

        auto permanent = DbParsingUtils::object_flag_to_game(db::ObjectFlag::Permanent);
        REQUIRE(permanent.has_value());
        REQUIRE(*permanent == ObjectFlag::Permanent);

        auto temporary = DbParsingUtils::object_flag_to_game(db::ObjectFlag::Temporary);
        REQUIRE(temporary.has_value());
        REQUIRE(*temporary == ObjectFlag::Temporary);

        auto decomposing = DbParsingUtils::object_flag_to_game(db::ObjectFlag::Decomposing);
        REQUIRE(decomposing.has_value());
        REQUIRE(*decomposing == ObjectFlag::Decomposing);

        auto float_flag = DbParsingUtils::object_flag_to_game(db::ObjectFlag::Float);
        REQUIRE(float_flag.has_value());
        REQUIRE(*float_flag == ObjectFlag::Float);
    }

    SECTION("DB flags without game equivalents return nullopt") {
        auto buoyant = DbParsingUtils::object_flag_to_game(db::ObjectFlag::Buoyant);
        REQUIRE_FALSE(buoyant.has_value());

        auto vehicle = DbParsingUtils::object_flag_to_game(db::ObjectFlag::Vehicle);
        REQUIRE_FALSE(vehicle.has_value());

        auto soulbound = DbParsingUtils::object_flag_to_game(db::ObjectFlag::Soulbound);
        REQUIRE_FALSE(soulbound.has_value());
    }

    SECTION("DB string parsing chains correctly to game flags") {
        // Full pipeline: DB string -> db::ObjectFlag -> game ObjectFlag
        auto db_flag = db::object_flag_from_db("GLOW");
        REQUIRE(db_flag.has_value());
        auto game_flag = DbParsingUtils::object_flag_to_game(*db_flag);
        REQUIRE(game_flag.has_value());
        REQUIRE(*game_flag == ObjectFlag::Glow);

        db_flag = db::object_flag_from_db("FLOAT");
        REQUIRE(db_flag.has_value());
        game_flag = DbParsingUtils::object_flag_to_game(*db_flag);
        REQUIRE(game_flag.has_value());
        REQUIRE(*game_flag == ObjectFlag::Float);

        db_flag = db::object_flag_from_db("INVISIBLE");
        REQUIRE(db_flag.has_value());
        game_flag = DbParsingUtils::object_flag_to_game(*db_flag);
        REQUIRE(game_flag.has_value());
        REQUIRE(*game_flag == ObjectFlag::Invisible);
    }

    SECTION("Enum values do NOT match between DB and game (validates the fix)") {
        // This is the core of the bug: static_cast would have mapped these wrong
        // db::ObjectFlag::Invisible = 2, but ObjectFlag::Invisible = 5
        REQUIRE(std::to_underlying(db::ObjectFlag::Invisible) != std::to_underlying(ObjectFlag::Invisible));

        // db::ObjectFlag::Float = 7, but ObjectFlag::Float = 31
        REQUIRE(std::to_underlying(db::ObjectFlag::Float) != std::to_underlying(ObjectFlag::Float));

        // db::ObjectFlag::Temporary = 5, but ObjectFlag::Temporary = 2
        REQUIRE(std::to_underlying(db::ObjectFlag::Temporary) != std::to_underlying(ObjectFlag::Temporary));

        // db::ObjectFlag::Magic = 3, but ObjectFlag::Magic = 6
        REQUIRE(std::to_underlying(db::ObjectFlag::Magic) != std::to_underlying(ObjectFlag::Magic));

        // db::ObjectFlag::Decomposing = 6, but ObjectFlag::Decomposing = 30
        REQUIRE(std::to_underlying(db::ObjectFlag::Decomposing) != std::to_underlying(ObjectFlag::Decomposing));
    }
}

// =============================================================================
// Object Flag Storage Tests
// =============================================================================

TEST_CASE("ObjectFlags: has_flag and set_flag round-trip", "[objects][flags][unit]") {
    SECTION("New object has no flags") {
        auto obj = ObjectBuilder().named("Plain Sword").as_weapon().build();
        REQUIRE_FALSE(obj->has_flag(ObjectFlag::Glow));
        REQUIRE_FALSE(obj->has_flag(ObjectFlag::Hum));
        REQUIRE_FALSE(obj->has_flag(ObjectFlag::Float));
        REQUIRE_FALSE(obj->has_flag(ObjectFlag::Magic));
        REQUIRE_FALSE(obj->has_flag(ObjectFlag::Invisible));
    }

    SECTION("Setting a flag makes has_flag return true") {
        auto obj = ObjectBuilder().named("Glowing Sword").as_weapon().build();
        obj->set_flag(ObjectFlag::Glow);
        REQUIRE(obj->has_flag(ObjectFlag::Glow));
        REQUIRE_FALSE(obj->has_flag(ObjectFlag::Hum));
    }

    SECTION("Multiple flags can be set independently") {
        auto obj = ObjectBuilder().named("Magic Belt").build();
        obj->set_flag(ObjectFlag::Glow);
        obj->set_flag(ObjectFlag::Hum);
        obj->set_flag(ObjectFlag::Float);
        obj->set_flag(ObjectFlag::Magic);

        REQUIRE(obj->has_flag(ObjectFlag::Glow));
        REQUIRE(obj->has_flag(ObjectFlag::Hum));
        REQUIRE(obj->has_flag(ObjectFlag::Float));
        REQUIRE(obj->has_flag(ObjectFlag::Magic));
        REQUIRE_FALSE(obj->has_flag(ObjectFlag::Invisible));
    }

    SECTION("Clearing a flag makes has_flag return false") {
        auto obj = ObjectBuilder().named("Fading Sword").as_weapon().build();
        obj->set_flag(ObjectFlag::Glow);
        REQUIRE(obj->has_flag(ObjectFlag::Glow));

        obj->set_flag(ObjectFlag::Glow, false);
        REQUIRE_FALSE(obj->has_flag(ObjectFlag::Glow));
    }

    SECTION("ObjectBuilder with_flag sets flags correctly") {
        auto obj = ObjectBuilder()
                       .named("Enchanted Cloak")
                       .as_armor()
                       .with_flag(ObjectFlag::Glow)
                       .with_flag(ObjectFlag::Magic)
                       .with_flag(ObjectFlag::Float)
                       .build();

        REQUIRE(obj->has_flag(ObjectFlag::Glow));
        REQUIRE(obj->has_flag(ObjectFlag::Magic));
        REQUIRE(obj->has_flag(ObjectFlag::Float));
        REQUIRE_FALSE(obj->has_flag(ObjectFlag::Hum));
    }

    SECTION("flags() returns all set flags") {
        auto obj = ObjectBuilder().named("Multi-Flag Item").build();
        obj->set_flag(ObjectFlag::Glow);
        obj->set_flag(ObjectFlag::Hum);
        obj->set_flag(ObjectFlag::Float);

        const auto &flags = obj->flags();
        REQUIRE(flags.size() == 3);
        REQUIRE(flags.contains(ObjectFlag::Glow));
        REQUIRE(flags.contains(ObjectFlag::Hum));
        REQUIRE(flags.contains(ObjectFlag::Float));
    }
}

// =============================================================================
// Flag Indicators Display Tests
// =============================================================================

TEST_CASE("ObjectFlags: flag_indicators produces correct display strings", "[objects][flags][display][unit]") {
    SECTION("Object with no flags returns empty string") {
        auto obj = ObjectBuilder().named("Plain Sword").as_weapon().build();
        REQUIRE(obj->flag_indicators() == "");
    }

    SECTION("Glowing object shows magenta indicator") {
        auto obj = ObjectBuilder().named("Glowing Gem").with_flag(ObjectFlag::Glow).build();
        auto indicators = obj->flag_indicators();
        REQUIRE(indicators.find("(glowing)") != std::string::npos);
        REQUIRE(indicators.find("magenta") != std::string::npos);
    }

    SECTION("Humming object shows cyan indicator") {
        auto obj = ObjectBuilder().named("Humming Blade").with_flag(ObjectFlag::Hum).build();
        auto indicators = obj->flag_indicators();
        REQUIRE(indicators.find("(humming)") != std::string::npos);
        REQUIRE(indicators.find("cyan") != std::string::npos);
    }

    SECTION("Floating object shows blue indicator") {
        auto obj = ObjectBuilder().named("Floating Orb").with_flag(ObjectFlag::Float).build();
        auto indicators = obj->flag_indicators();
        REQUIRE(indicators.find("(floating)") != std::string::npos);
        REQUIRE(indicators.find("blue") != std::string::npos);
    }

    SECTION("Invisible object shows dim indicator") {
        auto obj = ObjectBuilder().named("Hidden Ring").with_flag(ObjectFlag::Invisible).build();
        auto indicators = obj->flag_indicators();
        REQUIRE(indicators.find("(invisible)") != std::string::npos);
        REQUIRE(indicators.find("dim") != std::string::npos);
    }

    SECTION("Decomposing object shows dim indicator") {
        auto obj = ObjectBuilder().named("Rotting Corpse").with_flag(ObjectFlag::Decomposing).build();
        auto indicators = obj->flag_indicators();
        REQUIRE(indicators.find("(decomposing)") != std::string::npos);
        REQUIRE(indicators.find("dim") != std::string::npos);
    }

    SECTION("Multiple flags show all indicators") {
        auto obj = ObjectBuilder()
                       .named("Belt of GODLYNESS!")
                       .with_flag(ObjectFlag::Glow)
                       .with_flag(ObjectFlag::Hum)
                       .with_flag(ObjectFlag::Float)
                       .build();
        auto indicators = obj->flag_indicators();
        REQUIRE(indicators.find("(glowing)") != std::string::npos);
        REQUIRE(indicators.find("(humming)") != std::string::npos);
        REQUIRE(indicators.find("(floating)") != std::string::npos);
    }

    SECTION("Detection-based flags require viewer with detection") {
        auto obj = ObjectBuilder().named("Magic Sword").with_flag(ObjectFlag::Magic).build();

        // Without a viewer, magic flag should not show
        auto indicators_no_viewer = obj->flag_indicators(nullptr);
        REQUIRE(indicators_no_viewer.find("(magic)") == std::string::npos);
    }
}
