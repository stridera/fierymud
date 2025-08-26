/***************************************************************************
 *   File: tests/test_mobile_keywords.cpp               Part of FieryMUD *
 *  Usage: Test that mobile keyword parsing works correctly                *
 ***************************************************************************/

#include "../src/core/actor.hpp"
#include "../src/core/ids.hpp"
#include "mock_game_session.hpp"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("Mobile Keywords: Mobile::create() parses keywords from name", "[mobile][keywords][unit]") {
    UnifiedTestHarness::run_unit_test([&]() {
        // Create a mobile with multiple keywords in the name
        auto mobile_result = Mobile::create(EntityId{1001}, "guard town soldier", 5);
        REQUIRE(mobile_result.has_value());
        auto mobile = std::move(mobile_result.value());
        
        // Verify basic properties
        REQUIRE(mobile->id() == EntityId{1001});
        REQUIRE(mobile->name() == "guard town soldier");
        REQUIRE(mobile->stats().level == 5);
        
        // Verify keywords were parsed from the name
        REQUIRE(mobile->matches_keyword("guard"));
        REQUIRE(mobile->matches_keyword("town"));
        REQUIRE(mobile->matches_keyword("soldier"));
        
        // Verify case insensitive matching
        REQUIRE(mobile->matches_keyword("GUARD"));
        REQUIRE(mobile->matches_keyword("Town"));
        REQUIRE(mobile->matches_keyword("SOLDIER"));
        
        // Verify non-matching keywords
        REQUIRE_FALSE(mobile->matches_keyword("wizard"));
        REQUIRE_FALSE(mobile->matches_keyword("orc"));
        REQUIRE_FALSE(mobile->matches_keyword("nonexistent"));
    });
}

TEST_CASE("Mobile Keywords: JSON loading preserves keywords", "[mobile][keywords][json]") {
    UnifiedTestHarness::run_unit_test([&]() {
        // Create a mobile from JSON like world files do
        nlohmann::json mobile_json = {
            {"id", "2001"},
            {"type", "NPC"},  // Using NPC type since that's what mobiles are in world files
            {"name_list", "orc warrior fighter"},
            {"short_description", "a fierce orc warrior"},
            {"level", 8}
        };
        
        auto mobile_result = Mobile::from_json(mobile_json);
        REQUIRE(mobile_result.has_value());
        auto mobile = std::move(mobile_result.value());
        
        // Verify basic properties
        REQUIRE(mobile->id() == EntityId{2001});
        REQUIRE(mobile->name() == "orc warrior fighter");
        REQUIRE(mobile->short_description() == "a fierce orc warrior");
        REQUIRE(mobile->stats().level == 8);
        
        // Verify keywords from JSON
        REQUIRE(mobile->matches_keyword("orc"));
        REQUIRE(mobile->matches_keyword("warrior"));
        REQUIRE(mobile->matches_keyword("fighter"));
        
        // Verify case insensitive matching
        REQUIRE(mobile->matches_keyword("ORC"));
        REQUIRE(mobile->matches_keyword("Warrior"));
        REQUIRE(mobile->matches_keyword("FIGHTER"));
        
        // Verify non-matching keywords
        REQUIRE_FALSE(mobile->matches_keyword("elf"));
        REQUIRE_FALSE(mobile->matches_keyword("guard"));
    });
}