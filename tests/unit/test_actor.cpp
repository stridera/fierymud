#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include <nlohmann/json.hpp>

#include "core/actor.hpp"
#include "core/mobile.hpp"
#include "core/player.hpp"

using json = nlohmann::json;

TEST_CASE("Mobile JSON parsing with money handling", "[core][actor][mobile][json]") {

    SECTION("Parse mobile with string money values") {
        json mobile_json = {{"id", 1000},
                            {"name", "test mobile"},
                            {"level", 5},
                            {"alignment", 0},
                            {"money", {{"copper", "10"}, {"silver", "5"}, {"gold", "2"}, {"platinum", "1"}}}};

        auto result = Mobile::from_json(mobile_json);
        REQUIRE(result.has_value());
        auto mobile = std::move(result.value());

        // Verify money calculation: 10 + (5*10) + (2*100) + (1*1000) = 1260 copper
        REQUIRE(mobile->stats().gold == 1260);
    }

    SECTION("Parse mobile with numeric money values") {
        json mobile_json = {{"id", 1001},
                            {"name", "numeric mobile"},
                            {"level", 3},
                            {"alignment", 100},
                            {"money", {{"copper", 15}, {"silver", 8}, {"gold", 3}, {"platinum", 0}}}};

        auto result = Mobile::from_json(mobile_json);
        REQUIRE(result.has_value());
        auto mobile = std::move(result.value());

        // Verify money calculation: 15 + (8*10) + (3*100) + (0*1000) = 395 copper
        REQUIRE(mobile->stats().gold == 395);
    }

    SECTION("Parse mobile with mixed money value types") {
        json mobile_json = {{"id", 1002},
                            {"name", "mixed mobile"},
                            {"level", 1},
                            {"alignment", -500},
                            {"money", {{"copper", "0"}, {"silver", "0"}, {"gold", 0}, {"platinum", 0}}}};

        auto result = Mobile::from_json(mobile_json);
        REQUIRE(result.has_value());
        auto mobile = std::move(result.value());

        REQUIRE(mobile->stats().gold == 0);
    }

    SECTION("Parse mobile with negative money values (should convert to 0)") {
        json mobile_json = {{"id", 1003},
                            {"name", "negative money mobile"},
                            {"level", 2},
                            {"alignment", 0},
                            {"money", {{"copper", "-1"}, {"silver", "-5"}, {"gold", -2}, {"platinum", -1}}}};

        auto result = Mobile::from_json(mobile_json);
        REQUIRE(result.has_value());
        auto mobile = std::move(result.value());

        // All negative values should be converted to 0
        REQUIRE(mobile->stats().gold == 0);
    }

    SECTION("Parse mobile with missing money fields") {
        json mobile_json = {{"id", 1004}, {"name", "no money mobile"}, {"level", 1}, {"alignment", 0}};

        auto result = Mobile::from_json(mobile_json);
        REQUIRE(result.has_value());
        auto mobile = std::move(result.value());

        // Should default to 0 when money object is missing
        REQUIRE(mobile->stats().gold == 0);
    }

    SECTION("Parse mobile with partial money fields") {
        json mobile_json = {{"id", 1005},
                            {"name", "partial money mobile"},
                            {"level", 4},
                            {"alignment", 200},
                            {"money",
                             {
                                 {"gold", "5"} // Missing copper, silver, platinum
                             }}};

        auto result = Mobile::from_json(mobile_json);
        REQUIRE(result.has_value());
        auto mobile = std::move(result.value());

        // Only gold: 5*100 = 500 copper
        REQUIRE(mobile->stats().gold == 500);
    }
}

TEST_CASE("Mobile JSON parsing with level handling", "[core][actor][mobile][level]") {

    SECTION("Parse mobile with level 0 (should convert to 1)") {
        json mobile_json = {{"id", 2000}, {"name", "zero level mobile"}, {"level", 0}, {"alignment", 0}};

        auto result = Mobile::from_json(mobile_json);
        REQUIRE(result.has_value());
        auto mobile = std::move(result.value());

        // Level 0 should be converted to 1
        REQUIRE(mobile->stats().level == 1);
    }

    SECTION("Parse mobile with negative level (should convert to 1)") {
        json mobile_json = {{"id", 2001}, {"name", "negative level mobile"}, {"level", -5}, {"alignment", 0}};

        auto result = Mobile::from_json(mobile_json);
        REQUIRE(result.has_value());
        auto mobile = std::move(result.value());

        REQUIRE(mobile->stats().level == 1);
    }

    SECTION("Parse mobile with valid level") {
        json mobile_json = {{"id", 2002}, {"name", "normal level mobile"}, {"level", 25}, {"alignment", 0}};

        auto result = Mobile::from_json(mobile_json);
        REQUIRE(result.has_value());
        auto mobile = std::move(result.value());

        REQUIRE(mobile->stats().level == 25);
    }
}

TEST_CASE("Mobile JSON parsing with flags", "[core][actor][mobile][flags]") {

    SECTION("Parse mobile with mob flags") {
        json mobile_json = {{"id", 3000},
                            {"name", "flagged mobile"},
                            {"level", 10},
                            {"alignment", 0},
                            {"mob_flags", "SENTINEL, ISNPC, AWARE"}};

        auto result = Mobile::from_json(mobile_json);
        REQUIRE(result.has_value());
        auto mobile = std::move(result.value());

        // Verify mobile is created successfully (specific flag testing would need ActorFlag enum access)
        REQUIRE(mobile->name() == "flagged mobile");
        REQUIRE(mobile->stats().level == 10);
    }

    SECTION("Parse mobile with effect flags") {
        json mobile_json = {{"id", 3001},
                            {"name", "effect mobile"},
                            {"level", 8},
                            {"alignment", 0},
                            {"effect_flags", "DETECT_INVIS, INFRAVISION"}};

        auto result = Mobile::from_json(mobile_json);
        REQUIRE(result.has_value());
        auto mobile = std::move(result.value());

        REQUIRE(mobile->name() == "effect mobile");
    }

    SECTION("Parse mobile with empty flags") {
        json mobile_json = {{"id", 3002},     {"name", "no flags mobile"}, {"level", 5},
                            {"alignment", 0}, {"mob_flags", ""},           {"effect_flags", ""}};

        auto result = Mobile::from_json(mobile_json);
        REQUIRE(result.has_value());
        auto mobile = std::move(result.value());

        REQUIRE(mobile->name() == "no flags mobile");
    }
}

TEST_CASE("Mobile JSON parsing error handling", "[core][actor][mobile][errors]") {

    SECTION("Enhanced error reporting for mob_flags parsing") {
        // This tests the enhanced error reporting I added
        json mobile_json = {
            {"id", 4000}, {"name", "error mobile"}, {"level", 1}, {"alignment", 0}, {"mob_flags", 123}
            // Invalid type - number instead of string
        };

        // Should throw an exception with enhanced error reporting
        REQUIRE_THROWS_WITH(Mobile::from_json(mobile_json), Catch::Matchers::ContainsSubstring("mob_flags"));
    }

    SECTION("Enhanced error reporting for effect_flags parsing") {
        json mobile_json = {
            {"id", 4001}, {"name", "error mobile 2"}, {"level", 1}, {"alignment", 0}, {"effect_flags", 456}
            // Invalid type - number instead of string
        };

        // Should throw an exception with enhanced error reporting
        REQUIRE_THROWS_WITH(Mobile::from_json(mobile_json), Catch::Matchers::ContainsSubstring("effect_flags"));
    }
}

TEST_CASE("Stats validation with fixed gold handling", "[core][actor][stats][validation]") {

    SECTION("Stats with positive gold should validate") {
        Stats stats;
        stats.gold = 1000;
        stats.level = 5;
        stats.experience = 100;

        auto result = stats.validate();
        REQUIRE(result.has_value());
    }

    SECTION("Stats with zero gold should validate") {
        Stats stats;
        stats.gold = 0;
        stats.level = 1;
        stats.experience = 0;

        auto result = stats.validate();
        REQUIRE(result.has_value());
    }

    SECTION("Stats with negative gold should fail validation") {
        Stats stats;
        stats.gold = -100; // This should fail validation
        stats.level = 1;
        stats.experience = 0;

        auto result = stats.validate();
        REQUIRE_FALSE(result.has_value());
        REQUIRE(result.error().message.find("Gold cannot be negative") != std::string::npos);
    }
}
