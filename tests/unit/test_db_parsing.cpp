/***************************************************************************
 *   File: tests/unit/test_db_parsing.cpp                  Part of FieryMUD *
 *  Usage: Unit tests for database parsing utilities                        *
 ***************************************************************************/

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

TEST_CASE("DbParsing: Room Flag Parsing", "[database][parsing][roomflag]") {
    SECTION("Common room flags") {
        auto dark = DbParsingUtils::room_flag_from_db_string("DARK");
        REQUIRE(dark.has_value());
        REQUIRE(dark.value() == RoomFlag::Dark);

        auto peaceful = DbParsingUtils::room_flag_from_db_string("PEACEFUL");
        REQUIRE(peaceful.has_value());
        REQUIRE(peaceful.value() == RoomFlag::Peaceful);

        auto no_mob = DbParsingUtils::room_flag_from_db_string("NO_MOB");
        REQUIRE(no_mob.has_value());
        REQUIRE(no_mob.value() == RoomFlag::NoMob);
    }

    SECTION("Magic and teleport restrictions") {
        auto no_magic = DbParsingUtils::room_flag_from_db_string("NO_MAGIC");
        REQUIRE(no_magic.has_value());
        REQUIRE(no_magic.value() == RoomFlag::NoMagic);

        auto no_recall = DbParsingUtils::room_flag_from_db_string("NO_RECALL");
        REQUIRE(no_recall.has_value());
        REQUIRE(no_recall.value() == RoomFlag::NoRecall);

        auto no_summon = DbParsingUtils::room_flag_from_db_string("NO_SUMMON");
        REQUIRE(no_summon.has_value());
        REQUIRE(no_summon.value() == RoomFlag::NoSummon);

        auto no_teleport = DbParsingUtils::room_flag_from_db_string("NO_TELEPORT");
        REQUIRE(no_teleport.has_value());
        REQUIRE(no_teleport.value() == RoomFlag::NoTeleport);
    }

    SECTION("Special room types") {
        auto godroom = DbParsingUtils::room_flag_from_db_string("GODROOM");
        REQUIRE(godroom.has_value());
        REQUIRE(godroom.value() == RoomFlag::Godroom);

        auto arena = DbParsingUtils::room_flag_from_db_string("ARENA");
        REQUIRE(arena.has_value());
        REQUIRE(arena.value() == RoomFlag::Arena);

        auto bank = DbParsingUtils::room_flag_from_db_string("BANK");
        REQUIRE(bank.has_value());
        REQUIRE(bank.value() == RoomFlag::Bank);
    }

    SECTION("Elemental sector flags") {
        auto earth = DbParsingUtils::room_flag_from_db_string("EARTH_SECT");
        REQUIRE(earth.has_value());
        REQUIRE(earth.value() == RoomFlag::Earth);

        auto air = DbParsingUtils::room_flag_from_db_string("AIR_SECT");
        REQUIRE(air.has_value());
        REQUIRE(air.value() == RoomFlag::Air);

        auto fire = DbParsingUtils::room_flag_from_db_string("FIRE_SECT");
        REQUIRE(fire.has_value());
        REQUIRE(fire.value() == RoomFlag::Fire);

        auto water = DbParsingUtils::room_flag_from_db_string("WATER_SECT");
        REQUIRE(water.has_value());
        REQUIRE(water.value() == RoomFlag::Water);
    }

    SECTION("Unknown flag returns nullopt") {
        auto unknown = DbParsingUtils::room_flag_from_db_string("UNKNOWN_FLAG");
        REQUIRE_FALSE(unknown.has_value());

        auto empty = DbParsingUtils::room_flag_from_db_string("");
        REQUIRE_FALSE(empty.has_value());
    }
}
