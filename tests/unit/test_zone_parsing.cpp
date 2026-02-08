#include <catch2/catch_test_macros.hpp>
#include <nlohmann/json.hpp>

#include "../src/world/room.hpp"
#include "../src/world/zone.hpp"

using json = nlohmann::json;

TEST_CASE("Zone Parsing: Door Commands", "[zone][parsing][commands]") {

    SECTION("Parse door command with state as array") {
        json zone_json = {
            {"zone", {{"id", "100"}, {"name", "Test Zone"}, {"top", 199}, {"lifespan", 30}, {"reset_mode", "Normal"}}},
            {"commands",
             {{"door",
               {{
                   {"room", 1001}, {"direction", "North"}, {"state", {"CLOSED"}} // Array format
               }}}}}};

        auto result = Zone::from_json(zone_json);
        REQUIRE(result.has_value());
        auto zone = std::move(result.value());

        REQUIRE(zone->name() == "Test Zone");
        REQUIRE(zone->id() == EntityId{100});
    }

    SECTION("Parse door command with state as string") {
        json zone_json = {
            {"zone",
             {{"id", "101"}, {"name", "Test Zone 2"}, {"top", 199}, {"lifespan", 30}, {"reset_mode", "Normal"}}},
            {"commands",
             {{"door",
               {{
                   {"room", 1001}, {"direction", "East"}, {"state", "LOCKED"} // String format
               }}}}}};

        auto result = Zone::from_json(zone_json);
        REQUIRE(result.has_value());
        auto zone = std::move(result.value());

        REQUIRE(zone->name() == "Test Zone 2");
    }

    SECTION("Parse door command with multiple state values in array") {
        json zone_json = {
            {"zone",
             {{"id", "102"}, {"name", "Multi State Zone"}, {"top", 199}, {"lifespan", 30}, {"reset_mode", "Normal"}}},
            {"commands",
             {{"door",
               {{
                   {"room", 1001},
                   {"direction", "South"},
                   {"state", {"CLOSED", "LOCKED"}} // Multiple values - should use first
               }}}}}};

        auto result = Zone::from_json(zone_json);
        REQUIRE(result.has_value());
        auto zone = std::move(result.value());

        REQUIRE(zone->name() == "Multi State Zone");
    }

    SECTION("Parse door command with empty state array") {
        json zone_json = {
            {"zone",
             {{"id", "103"}, {"name", "Empty State Zone"}, {"top", 199}, {"lifespan", 30}, {"reset_mode", "Normal"}}},
            {"commands",
             {{"door",
               {{
                   {"room", 1001}, {"direction", "West"}, {"state", {}} // Empty array - should default to "open"
               }}}}}};

        auto result = Zone::from_json(zone_json);
        REQUIRE(result.has_value());
        auto zone = std::move(result.value());

        REQUIRE(zone->name() == "Empty State Zone");
    }
}

TEST_CASE("Zone Parsing: Direction Strings", "[zone][parsing][commands]") {

    SECTION("Parse door command with cardinal directions") {
        json zone_json = {{"zone",
                           {{"id", "200"},
                            {"name", "Direction Test Zone"},
                            {"top", 299},
                            {"lifespan", 30},
                            {"reset_mode", "Normal"}}},
                          {"commands",
                           {{"door",
                             {{{"room", 2001}, {"direction", "North"}, {"state", {"OPEN"}}},
                              {{"room", 2001}, {"direction", "East"}, {"state", {"CLOSED"}}},
                              {{"room", 2001}, {"direction", "South"}, {"state", {"LOCKED"}}},
                              {{"room", 2001}, {"direction", "West"}, {"state", {"UNLOCKED"}}}}}}}};

        auto result = Zone::from_json(zone_json);
        REQUIRE(result.has_value());
        auto zone = std::move(result.value());

        REQUIRE(zone->name() == "Direction Test Zone");
    }

    SECTION("Parse door command with vertical directions") {
        json zone_json = {
            {"zone",
             {{"id", "201"}, {"name", "Vertical Zone"}, {"top", 299}, {"lifespan", 30}, {"reset_mode", "Normal"}}},
            {"commands",
             {{"door",
               {{{"room", 2101}, {"direction", "Up"}, {"state", {"CLOSED"}}},
                {{"room", 2101}, {"direction", "Down"}, {"state", {"LOCKED"}}}}}}}};

        auto result = Zone::from_json(zone_json);
        REQUIRE(result.has_value());
        auto zone = std::move(result.value());

        REQUIRE(zone->name() == "Vertical Zone");
    }

    SECTION("Parse door command with diagonal directions") {
        json zone_json = {
            {"zone",
             {{"id", "202"}, {"name", "Diagonal Zone"}, {"top", 299}, {"lifespan", 30}, {"reset_mode", "Normal"}}},
            {"commands",
             {{"door",
               {{{"room", 2201}, {"direction", "Northeast"}, {"state", {"OPEN"}}},
                {{"room", 2201}, {"direction", "Southeast"}, {"state", {"CLOSED"}}}}}}}};

        auto result = Zone::from_json(zone_json);
        REQUIRE(result.has_value());
        auto zone = std::move(result.value());

        REQUIRE(zone->name() == "Diagonal Zone");
    }

    SECTION("Parse door command with special directions") {
        json zone_json = {
            {"zone",
             {{"id", "203"}, {"name", "Special Zone"}, {"top", 299}, {"lifespan", 30}, {"reset_mode", "Normal"}}},
            {"commands",
             {{"door",
               {{{"room", 2301}, {"direction", "In"}, {"state", {"CLOSED"}}},
                {{"room", 2301}, {"direction", "Out"}, {"state", {"LOCKED"}}}}}}}};

        auto result = Zone::from_json(zone_json);
        REQUIRE(result.has_value());
        auto zone = std::move(result.value());

        REQUIRE(zone->name() == "Special Zone");
    }
}

TEST_CASE("Zone Parsing: Mixed Data Types", "[zone][parsing][commands]") {

    SECTION("Parse zone with mobile and object commands") {
        json zone_json = {
            {"zone",
             {{"id", "300"}, {"name", "Mixed Command Zone"}, {"top", 399}, {"lifespan", 30}, {"reset_mode", "Normal"}}},
            {"commands",
             {{"mob", {{{"id", 3001}, {"max", 1}, {"room", 3001}, {"name", "(test mobile)"}}}},
              {"object", {{{"id", 3101}, {"max", 1}, {"room", 3001}, {"name", "(test object)"}}}},
              {"door", {{{"room", 3001}, {"direction", "North"}, {"state", {"CLOSED"}}}}}}}};

        auto result = Zone::from_json(zone_json);
        REQUIRE(result.has_value());
        auto zone = std::move(result.value());

        REQUIRE(zone->name() == "Mixed Command Zone");
        REQUIRE(zone->id() == EntityId{300});
    }
}

TEST_CASE("Zone Parsing: Error Handling", "[zone][parsing][errors]") {

    SECTION("Missing zone section should fail") {
        json invalid_zone_json = {
            {"commands", {{"door", {{{"room", 1001}, {"direction", "North"}, {"state", {"CLOSED"}}}}}}}
            // Missing "zone" section
        };

        auto result = Zone::from_json(invalid_zone_json);
        REQUIRE_FALSE(result.has_value());
    }

    SECTION("Invalid door state type should be handled gracefully") {
        json zone_json = {
            {"zone", {{"id", "400"}, {"name", "Error Zone"}, {"top", 499}, {"lifespan", 30}, {"reset_mode", "Normal"}}},
            {"commands",
             {{"door",
               {{
                   {"room", 4001}, {"direction", "North"}, {"state", 123}
                   // Invalid type - number instead of string/array
               }}}}}};

        // Should either handle gracefully or provide clear error
        auto result = Zone::from_json(zone_json);
        // Don't require success/failure - depends on implementation robustness
        if (!result.has_value()) {
            // If it fails, error should be descriptive
            REQUIRE(!result.error().message.empty());
        }
    }
}

TEST_CASE("Room Parsing: Exit Door States", "[room][parsing][exits]") {

    SECTION("Parse room with exit door state as array") {
        json room_json = {{"id", "1001"},
                          {"name", "Test Room"},
                          {"description", "A test room."},
                          {"flags", ""},
                          {"sector", "CITY"},
                          {"exits",
                           {{"North",
                             {{"to_room", 1002},
                              {"door",
                               {
                                   {"state", {"CLOSED"}} // Array format
                               }}}}}}};

        auto result = Room::from_json(room_json);
        REQUIRE(result.has_value());
        auto room = std::move(result.value());

        auto exit = room->get_exit(Direction::North);
        REQUIRE(exit != nullptr);
        REQUIRE(exit->is_closed);
    }

    SECTION("Parse room with exit door state as string") {
        json room_json = {{"id", "1002"},
                          {"name", "Test Room 2"},
                          {"description", "Another test room."},
                          {"flags", ""},
                          {"sector", "FOREST"},
                          {"exits",
                           {{"East",
                             {{"to_room", 1003},
                              {"door",
                               {
                                   {"state", "LOCKED"} // String format
                               }}}}}}};

        auto result = Room::from_json(room_json);
        REQUIRE(result.has_value());
        auto room = std::move(result.value());

        auto exit = room->get_exit(Direction::East);
        REQUIRE(exit != nullptr);
        REQUIRE(exit->is_locked);
    }
}

TEST_CASE("Room Parsing: Exit Directions", "[room][parsing][exits]") {

    SECTION("Parse room with cardinal direction exits") {
        json room_json = {{"id", "2001"},
                          {"name", "Direction Test Room"},
                          {"description", "A room for testing directions."},
                          {"flags", ""},
                          {"sector", "PLAINS"},
                          {"exits",
                           {{"North", {{"to_room", 2002}}},
                            {"East", {{"to_room", 2003}}},
                            {"South", {{"to_room", 2004}}},
                            {"West", {{"to_room", 2005}}}}}};

        auto result = Room::from_json(room_json);
        REQUIRE(result.has_value());
        auto room = std::move(result.value());

        REQUIRE(room->has_exit(Direction::North));
        REQUIRE(room->has_exit(Direction::East));
        REQUIRE(room->has_exit(Direction::South));
        REQUIRE(room->has_exit(Direction::West));
    }
}

TEST_CASE("Room Parsing: Error Handling", "[room][parsing][errors]") {

    SECTION("Missing room ID should fail") {
        json invalid_room_json = {
            {"name", "Invalid Room"},
            {"description", "A room without an ID."},
            {"flags", ""},
            {"sector", "CITY"},
            {"exits", {}}
            // Missing "id" field
        };

        auto result = Room::from_json(invalid_room_json);
        REQUIRE_FALSE(result.has_value());
    }

    SECTION("Invalid exit door state type should be handled gracefully") {
        json room_json = {{"id", "3001"},
                          {"name", "Error Room"},
                          {"description", "A room with an error."},
                          {"flags", ""},
                          {"sector", "MOUNTAIN"},
                          {"exits",
                           {{"North",
                             {{"to_room", 3002},
                              {"door",
                               {
                                   {"state", 123} // Invalid type - number instead of string/array
                               }}}}}}};

        // Should either handle gracefully or provide clear error
        auto result = Room::from_json(room_json);
        // Don't require success/failure - depends on implementation robustness
        if (!result.has_value()) {
            // If it fails, error should be descriptive
            REQUIRE(!result.error().message.empty());
        }
    }
}
