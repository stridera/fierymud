#include "../../src/database/db_parsing_utils.hpp"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("DbParsing: PostgreSQL Array Parsing", "[database][parsing][array]") {
    SECTION("Empty and null arrays") {
        auto empty = DbParsingUtils::parse_pg_array("");
        REQUIRE(empty.empty());

        auto braces_only = DbParsingUtils::parse_pg_array("{}");
        REQUIRE(braces_only.empty());
    }

    SECTION("Simple unquoted arrays") {
        auto single = DbParsingUtils::parse_pg_array("{NORTH}");
        REQUIRE(single.size() == 1);
        REQUIRE(single[0] == "NORTH");

        auto multiple = DbParsingUtils::parse_pg_array("{NORTH,SOUTH,EAST}");
        REQUIRE(multiple.size() == 3);
        REQUIRE(multiple[0] == "NORTH");
        REQUIRE(multiple[1] == "SOUTH");
        REQUIRE(multiple[2] == "EAST");
    }

    SECTION("Direction keywords from database") {
        // Typical room exit keywords
        auto keywords = DbParsingUtils::parse_pg_array("{door,gate}");
        REQUIRE(keywords.size() == 2);
        REQUIRE(keywords[0] == "door");
        REQUIRE(keywords[1] == "gate");
    }

    SECTION("Room flags from database") {
        auto flags = DbParsingUtils::parse_pg_array("{DARK,NO_MOB,PEACEFUL}");
        REQUIRE(flags.size() == 3);
        REQUIRE(flags[0] == "DARK");
        REQUIRE(flags[1] == "NO_MOB");
        REQUIRE(flags[2] == "PEACEFUL");
    }

    SECTION("Quoted strings with spaces") {
        auto quoted = DbParsingUtils::parse_pg_array("{\"wooden door\",\"iron gate\"}");
        REQUIRE(quoted.size() == 2);
        REQUIRE(quoted[0] == "wooden door");
        REQUIRE(quoted[1] == "iron gate");
    }

    SECTION("Mixed quoted and unquoted") {
        auto mixed = DbParsingUtils::parse_pg_array("{simple,\"with spaces\",another}");
        REQUIRE(mixed.size() == 3);
        REQUIRE(mixed[0] == "simple");
        REQUIRE(mixed[1] == "with spaces");
        REQUIRE(mixed[2] == "another");
    }

    SECTION("Escaped characters") {
        // Backslash-escaped quotes inside quoted strings
        auto escaped = DbParsingUtils::parse_pg_array("{\"test\\\"quoted\"}");
        REQUIRE(escaped.size() == 1);
        REQUIRE(escaped[0] == "test\"quoted");
    }

    SECTION("Array without braces (edge case)") {
        // Should still work if braces are missing
        auto no_braces = DbParsingUtils::parse_pg_array("NORTH,SOUTH");
        REQUIRE(no_braces.size() == 2);
        REQUIRE(no_braces[0] == "NORTH");
        REQUIRE(no_braces[1] == "SOUTH");
    }
}

TEST_CASE("DbParsing: Sector Type Parsing", "[database][parsing][sector]") {
    SECTION("Common sector types") {
        auto structure = DbParsingUtils::sector_from_db_string("STRUCTURE");
        REQUIRE(structure.has_value());
        REQUIRE(structure.value() == SectorType::Inside);

        auto city = DbParsingUtils::sector_from_db_string("CITY");
        REQUIRE(city.has_value());
        REQUIRE(city.value() == SectorType::City);

        auto forest = DbParsingUtils::sector_from_db_string("FOREST");
        REQUIRE(forest.has_value());
        REQUIRE(forest.value() == SectorType::Forest);

        auto mountain = DbParsingUtils::sector_from_db_string("MOUNTAIN");
        REQUIRE(mountain.has_value());
        REQUIRE(mountain.value() == SectorType::Mountains);
    }

    SECTION("Water and underwater sectors") {
        auto shallows = DbParsingUtils::sector_from_db_string("SHALLOWS");
        REQUIRE(shallows.has_value());
        REQUIRE(shallows.value() == SectorType::Water_Swim);

        auto water = DbParsingUtils::sector_from_db_string("WATER");
        REQUIRE(water.has_value());
        REQUIRE(water.value() == SectorType::Water_Noswim);

        auto underwater = DbParsingUtils::sector_from_db_string("UNDERWATER");
        REQUIRE(underwater.has_value());
        REQUIRE(underwater.value() == SectorType::Underwater);
    }

    SECTION("Planar sectors") {
        auto astral = DbParsingUtils::sector_from_db_string("ASTRALPLANE");
        REQUIRE(astral.has_value());
        REQUIRE(astral.value() == SectorType::Astral);

        auto fire = DbParsingUtils::sector_from_db_string("FIREPLANE");
        REQUIRE(fire.has_value());
        REQUIRE(fire.value() == SectorType::Fire);

        auto ethereal = DbParsingUtils::sector_from_db_string("ETHEREALPLANE");
        REQUIRE(ethereal.has_value());
        REQUIRE(ethereal.value() == SectorType::Spirit);
    }

    SECTION("Unknown sector returns nullopt") {
        auto unknown = DbParsingUtils::sector_from_db_string("UNKNOWN_SECTOR");
        REQUIRE_FALSE(unknown.has_value());

        auto empty = DbParsingUtils::sector_from_db_string("");
        REQUIRE_FALSE(empty.has_value());
    }
}

// Room Flag Parsing tests removed - RoomFlag replaced by baseLightLevel and Lua restrictions
