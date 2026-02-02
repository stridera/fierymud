#include "../src/world/room.hpp"
#include "../src/world/world_manager.hpp"
#include "../src/world/zone.hpp"
#include "../src/core/object.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <nlohmann/json.hpp>
#include <filesystem>
#include <fstream>

TEST_CASE("World Utils: Directions", "[world][utils][direction]") {
    SECTION("Direction name conversion") {
        REQUIRE(RoomUtils::get_direction_name(Direction::North) == "North");
        REQUIRE(RoomUtils::get_direction_name(Direction::Southeast) == "Southeast");
        REQUIRE(RoomUtils::get_direction_name(Direction::None) == "None");
    }

    SECTION("Direction parsing - lowercase") {
        auto north = RoomUtils::parse_direction("north");
        REQUIRE(north.has_value());
        REQUIRE(north.value() == Direction::North);

        auto n = RoomUtils::parse_direction("n");
        REQUIRE(n.has_value());
        REQUIRE(n.value() == Direction::North);

        auto invalid = RoomUtils::parse_direction("invalid");
        REQUIRE_FALSE(invalid.has_value());
    }

    SECTION("Direction parsing - uppercase (database format)") {
        // These tests verify the fix for PostgreSQL enum values which are stored uppercase
        auto north = RoomUtils::parse_direction("NORTH");
        REQUIRE(north.has_value());
        REQUIRE(north.value() == Direction::North);

        auto east = RoomUtils::parse_direction("EAST");
        REQUIRE(east.has_value());
        REQUIRE(east.value() == Direction::East);

        auto south = RoomUtils::parse_direction("SOUTH");
        REQUIRE(south.has_value());
        REQUIRE(south.value() == Direction::South);

        auto west = RoomUtils::parse_direction("WEST");
        REQUIRE(west.has_value());
        REQUIRE(west.value() == Direction::West);

        auto up = RoomUtils::parse_direction("UP");
        REQUIRE(up.has_value());
        REQUIRE(up.value() == Direction::Up);

        auto down = RoomUtils::parse_direction("DOWN");
        REQUIRE(down.has_value());
        REQUIRE(down.value() == Direction::Down);

        auto northeast = RoomUtils::parse_direction("NORTHEAST");
        REQUIRE(northeast.has_value());
        REQUIRE(northeast.value() == Direction::Northeast);

        auto southwest = RoomUtils::parse_direction("SOUTHWEST");
        REQUIRE(southwest.has_value());
        REQUIRE(southwest.value() == Direction::Southwest);
    }

    SECTION("Direction parsing - PascalCase (enum names)") {
        // magic_enum should handle the exact enum names
        auto north = RoomUtils::parse_direction("North");
        REQUIRE(north.has_value());
        REQUIRE(north.value() == Direction::North);

        auto southeast = RoomUtils::parse_direction("Southeast");
        REQUIRE(southeast.has_value());
        REQUIRE(southeast.value() == Direction::Southeast);
    }

    SECTION("Opposite directions") {
        REQUIRE(RoomUtils::get_opposite_direction(Direction::North) == Direction::South);
        REQUIRE(RoomUtils::get_opposite_direction(Direction::East) == Direction::West);
        REQUIRE(RoomUtils::get_opposite_direction(Direction::Up) == Direction::Down);
        REQUIRE(RoomUtils::get_opposite_direction(Direction::In) == Direction::Out);
    }

    SECTION("Direction abbreviations") {
        REQUIRE(RoomUtils::get_direction_abbrev(Direction::North) == "n");
        REQUIRE(RoomUtils::get_direction_abbrev(Direction::Northeast) == "ne");
        REQUIRE(RoomUtils::get_direction_abbrev(Direction::None) == "none");
    }
}

TEST_CASE("World Utils: ExitInfo", "[world][utils][exit]") {
    SECTION("Basic exit creation") {
        ExitInfo exit;
        exit.to_room = EntityId{1001};
        exit.description = "A wooden door leads north.";

        REQUIRE(exit.to_room.is_valid());
        REQUIRE(exit.is_passable());
        REQUIRE_FALSE(exit.has_functional_door());
    }

    SECTION("Door state functionality") {
        ExitInfo exit;
        exit.to_room = EntityId{1001};
        exit.has_door = true;
        exit.keyword = "door";
        exit.is_closed = true;

        REQUIRE(exit.has_functional_door());
        REQUIRE_FALSE(exit.is_passable());

        std::string state_desc = exit.door_state_description();
        REQUIRE(state_desc.find("closed") != std::string::npos);
        REQUIRE(state_desc.find("door") != std::string::npos);
    }

    SECTION("Exit JSON serialization") {
        ExitInfo exit;
        exit.to_room = EntityId{2001};
        exit.description = "Test exit";
        exit.has_door = true;
        exit.is_locked = true;
        exit.key_id = EntityId{3001};
        exit.difficulty = 15;

        auto json = exit.to_json();
        REQUIRE(json["to_room"] == 2001);
        REQUIRE(json["description"] == "Test exit");
        REQUIRE(json["has_door"] == true);
        REQUIRE(json["is_locked"] == true);
        REQUIRE(json["key_id"] == 3001);
        REQUIRE(json["difficulty"] == 15);

        auto restored_result = ExitInfo::from_json(json);
        REQUIRE(restored_result.has_value());

        auto restored = restored_result.value();
        REQUIRE(restored.to_room == exit.to_room);
        REQUIRE(restored.description == exit.description);
        REQUIRE(restored.has_door == exit.has_door);
        REQUIRE(restored.is_locked == exit.is_locked);
        REQUIRE(restored.key_id == exit.key_id);
        REQUIRE(restored.difficulty == exit.difficulty);
    }
}

TEST_CASE("World Utils: Room", "[world][utils][room]") {
    SECTION("Room creation") {
        auto room_result = Room::create(EntityId{1001}, "Test Room", SectorType::Inside);
        REQUIRE(room_result.has_value());

        auto room = std::move(room_result.value());
        REQUIRE(room->id() == EntityId{1001});
        REQUIRE(room->name() == "Test Room");
        REQUIRE(room->sector_type() == SectorType::Inside);
    }

    SECTION("Room validation") {
        auto invalid_room = Room::create(INVALID_ENTITY_ID, "Test", SectorType::Inside);
        REQUIRE_FALSE(invalid_room.has_value());

        auto empty_name = Room::create(EntityId{1001}, "", SectorType::Inside);
        REQUIRE_FALSE(empty_name.has_value());

        auto undefined_sector = Room::create(EntityId{1001}, "Test", SectorType::Undefined);
        REQUIRE_FALSE(undefined_sector.has_value());
    }

    SECTION("Room lighting management") {
        auto room = Room::create(EntityId{1001}, "Test Room", SectorType::Inside).value();

        // Indoor rooms start with ambient light (base_light_level 0)
        REQUIRE_FALSE(room->is_dark());

        // Set to dark (negative light level)
        room->set_base_light_level(-3);
        REQUIRE(room->is_dark());

        // Reset to ambient
        room->set_base_light_level(0);
        REQUIRE_FALSE(room->is_dark());
    }

    SECTION("Room state queries") {
        auto room = Room::create(EntityId{1001}, "Test Room", SectorType::Inside).value();

        REQUIRE_FALSE(room->is_peaceful());
        room->set_peaceful(true);
        REQUIRE(room->is_peaceful());

        REQUIRE(room->allows_magic());
        room->set_allows_magic(false);
        REQUIRE_FALSE(room->allows_magic());
    }
}

TEST_CASE("World Utils: Room Exit System", "[world][utils][room][exits]") {
    SECTION("Adding and removing exits") {
        auto room = Room::create(EntityId{1001}, "Test Room", SectorType::Inside).value();

        REQUIRE_FALSE(room->has_exit(Direction::North));

        ExitInfo exit;
        exit.to_room = EntityId{1002};
        exit.description = "North exit";

        auto result = room->set_exit(Direction::North, exit);
        REQUIRE(result.has_value());
        REQUIRE(room->has_exit(Direction::North));

        auto retrieved_exit = room->get_exit(Direction::North);
        REQUIRE(retrieved_exit != nullptr);
        REQUIRE(retrieved_exit->to_room == EntityId{1002});

        room->remove_exit(Direction::North);
        REQUIRE_FALSE(room->has_exit(Direction::North));
    }

    SECTION("Exit validation") {
        auto room = Room::create(EntityId{1001}, "Test Room", SectorType::Inside).value();

        ExitInfo invalid_exit;
        invalid_exit.to_room = INVALID_ENTITY_ID;

        auto result = room->set_exit(Direction::North, invalid_exit);
        REQUIRE_FALSE(result.has_value());

        auto none_direction = room->set_exit(Direction::None, invalid_exit);
        REQUIRE_FALSE(none_direction.has_value());
    }

    SECTION("Available and visible exits") {
        auto room = Room::create(EntityId{1001}, "Test Room", SectorType::Inside).value();

        // Add normal exit
        ExitInfo north_exit;
        north_exit.to_room = EntityId{1002};
        room->set_exit(Direction::North, north_exit);

        // Add hidden exit
        ExitInfo hidden_exit;
        hidden_exit.to_room = EntityId{1003};
        hidden_exit.is_hidden = true;
        room->set_exit(Direction::South, hidden_exit);

        auto available = room->get_available_exits();
        REQUIRE(available.size() == 2);
        REQUIRE(std::find(available.begin(), available.end(), Direction::North) != available.end());
        REQUIRE(std::find(available.begin(), available.end(), Direction::South) != available.end());

        auto visible = room->get_visible_exits();
        REQUIRE(visible.size() == 1);
        REQUIRE(std::find(visible.begin(), visible.end(), Direction::North) != visible.end());
        REQUIRE(std::find(visible.begin(), visible.end(), Direction::South) == visible.end());
    }
}

TEST_CASE("World Utils: Room Lighting System", "[world][utils][room][lighting]") {
    SECTION("Natural lighting") {
        auto outdoor_room = Room::create(EntityId{1001}, "Outdoor", SectorType::Field).value();
        auto indoor_room = Room::create(EntityId{1002}, "Indoor", SectorType::Inside).value();

        REQUIRE(outdoor_room->is_naturally_lit());
        REQUIRE_FALSE(indoor_room->is_naturally_lit());

        // Negative light level forces darkness
        outdoor_room->set_base_light_level(-3);
        REQUIRE_FALSE(outdoor_room->is_naturally_lit());
    }

    SECTION("Light level calculations") {
        auto room = Room::create(EntityId{1001}, "Test Room", SectorType::Inside).value();

        // Indoor room should have low light initially
        int initial_light = room->calculate_effective_light();

        // Set base light level
        room->set_base_light_level(10);
        REQUIRE(room->calculate_effective_light() >= initial_light + 10);

        // Negative light level means darkness
        room->set_base_light_level(-3);
        REQUIRE(room->is_dark());
    }
}

TEST_CASE("World Utils: Room Capacity and Occupancy", "[world][utils][room][capacity]") {
    SECTION("Room capacity limits") {
        auto normal_room = Room::create(EntityId{1001}, "Normal Room", SectorType::Inside).value();
        auto tunnel = Room::create(EntityId{1002}, "Tunnel", SectorType::Inside).value();
        auto private_room = Room::create(EntityId{1003}, "Private Room", SectorType::Inside).value();

        // Tunnel-like rooms have capacity 1
        tunnel->set_capacity(1);
        // Private rooms have capacity 2
        private_room->set_capacity(2);

        REQUIRE(normal_room->max_occupants() >= 10);  // Default capacity
        REQUIRE(tunnel->max_occupants() == 1);
        REQUIRE(private_room->max_occupants() == 2);
    }
}

TEST_CASE("World Utils: Room JSON Serialization", "[world][utils][room][json]") {
    SECTION("Complete room serialization") {
        auto room = Room::create(EntityId{1001}, "Test Room", SectorType::Forest).value();
        room->set_description("A dense forest with tall trees.");
        std::vector<std::string> keywords = {"forest", "trees", "dense"};
        room->set_keywords(keywords);
        room->set_base_light_level(5);
        room->set_zone_id(EntityId{100});
        room->set_allows_magic(false);
        room->set_peaceful(true);

        // Add exit
        ExitInfo exit;
        exit.to_room = EntityId{1002};
        exit.description = "Path to the north";
        exit.has_door = true;
        exit.keyword = "gate";
        room->set_exit(Direction::North, exit);

        auto json = room->to_json();

        REQUIRE(json["id"] == 1001);
        REQUIRE(json["name"] == "Test Room");
        REQUIRE(json["sector_type"] == "Forest");
        REQUIRE(json["base_light_level"] == 5);
        REQUIRE(json["zone_id"] == 100);

        REQUIRE(json["exits"].contains("North"));
        REQUIRE(json["exits"]["North"]["to_room"] == 1002);

        // Test round-trip
        auto restored_result = Room::from_json(json);
        REQUIRE(restored_result.has_value());

        auto restored = std::move(restored_result.value());
        REQUIRE(restored->id() == room->id());
        REQUIRE(restored->name() == room->name());
        REQUIRE(restored->sector_type() == room->sector_type());
        REQUIRE(restored->base_light_level() == room->base_light_level());
        REQUIRE(restored->zone_id() == room->zone_id());
        REQUIRE_FALSE(restored->allows_magic());
        REQUIRE(restored->is_peaceful());
        REQUIRE(restored->has_exit(Direction::North));
    }
}

TEST_CASE("World Utils: Zone", "[world][utils][zone]") {
    SECTION("Zone creation") {
        auto zone_result = Zone::create(EntityId{100}, "Test Zone", 30, ResetMode::Empty);
        REQUIRE(zone_result.has_value());

        auto zone = std::move(zone_result.value());
        REQUIRE(zone->id() == EntityId{100});
        REQUIRE(zone->name() == "Test Zone");
        REQUIRE(zone->reset_minutes() == 30);
        REQUIRE(zone->reset_mode() == ResetMode::Empty);
    }

    SECTION("Zone validation") {
        auto invalid_zone = Zone::create(INVALID_ENTITY_ID, "Test", 30);
        REQUIRE_FALSE(invalid_zone.has_value());

        auto empty_name = Zone::create(EntityId{100}, "", 30);
        REQUIRE_FALSE(empty_name.has_value());

        auto negative_reset = Zone::create(EntityId{100}, "Test", -5);
        REQUIRE_FALSE(negative_reset.has_value());
    }

    SECTION("Zone properties") {
        auto zone = Zone::create(EntityId{100}, "Test Zone").value();

        zone->set_min_level(10);
        zone->set_max_level(20);
        zone->set_builders("TestBuilder");

        REQUIRE(zone->min_level() == 10);
        REQUIRE(zone->max_level() == 20);
        REQUIRE(zone->builders() == "TestBuilder");
        REQUIRE(zone->allows_level(15));
        REQUIRE_FALSE(zone->allows_level(5));
        REQUIRE_FALSE(zone->allows_level(25));
    }
}

TEST_CASE("World Utils: Zone Flag Management", "[world][utils][zone][flags]") {
    SECTION("Basic flag operations") {
        auto zone = Zone::create(EntityId{100}, "Test Zone").value();

        REQUIRE_FALSE(zone->has_flag(ZoneFlag::Closed));
        REQUIRE(zone->allows_mortals());

        zone->set_flag(ZoneFlag::Closed);
        zone->set_flag(ZoneFlag::NoMortals);

        REQUIRE(zone->has_flag(ZoneFlag::Closed));
        REQUIRE(zone->has_flag(ZoneFlag::NoMortals));
        REQUIRE(zone->is_closed());
        REQUIRE_FALSE(zone->allows_mortals());

        zone->remove_flag(ZoneFlag::Closed);
        REQUIRE_FALSE(zone->is_closed());
    }

    SECTION("Zone state queries") {
        auto zone = Zone::create(EntityId{100}, "Test Zone").value();

        REQUIRE_FALSE(zone->is_quest_zone());
        zone->set_flag(ZoneFlag::Quest);
        REQUIRE(zone->is_quest_zone());

        REQUIRE(zone->allows_recall());
        zone->set_flag(ZoneFlag::Recall_Ok);
        REQUIRE(zone->allows_recall());

        REQUIRE(zone->allows_combat());
        zone->set_flag(ZoneFlag::Noattack);
        REQUIRE_FALSE(zone->allows_combat());
    }
}

TEST_CASE("World Utils: Zone Room Management", "[world][utils][zone][rooms]") {
    SECTION("Adding and removing rooms") {
        auto zone = Zone::create(EntityId{100}, "Test Zone").value();

        REQUIRE_FALSE(zone->contains_room(EntityId{1001}));
        REQUIRE(zone->rooms().empty());

        zone->add_room(EntityId{1001});
        zone->add_room(EntityId{1002});

        REQUIRE(zone->contains_room(EntityId{1001}));
        REQUIRE(zone->contains_room(EntityId{1002}));
        REQUIRE(zone->rooms().size() == 2);

        zone->remove_room(EntityId{1001});
        REQUIRE_FALSE(zone->contains_room(EntityId{1001}));
        REQUIRE(zone->rooms().size() == 1);
    }

    SECTION("Room range management") {
        auto zone = Zone::create(EntityId{100}, "Test Zone").value();

        zone->set_first_room(EntityId{1001});
        zone->set_last_room(EntityId{1099});

        REQUIRE(zone->first_room() == EntityId{1001});
        REQUIRE(zone->last_room() == EntityId{1099});
    }
}

TEST_CASE("World Utils: Zone Command System", "[world][utils][zone][commands]") {
    SECTION("Zone command creation") {
        ZoneCommand cmd;
        cmd.command_type = ZoneCommandType::Load_Mobile;
        cmd.if_flag = 1;
        cmd.entity_id = EntityId{3001}; // Mobile ID
        cmd.room_id = EntityId{1001};   // Room ID
        cmd.max_count = 5;              // Max count
        cmd.comment = "M 1 3001 1001 0 5";

        REQUIRE(cmd.should_execute());

        std::string cmd_str = cmd.to_string();
        REQUIRE(cmd_str.find("Load_Mobile") != std::string::npos);
    }

}

TEST_CASE("World Utils: Zone Reset System", "[world][utils][zone][reset]") {
    SECTION("Reset mode behavior") {
        auto never_zone = Zone::create(EntityId{100}, "Never Reset", 1, ResetMode::Never).value();
        auto manual_zone = Zone::create(EntityId{101}, "Manual Reset", 1, ResetMode::Manual).value();

        REQUIRE_FALSE(never_zone->needs_reset());
        REQUIRE_FALSE(manual_zone->needs_reset());

        // Test always reset mode
        auto always_zone = Zone::create(EntityId{102}, "Always Reset", 0, ResetMode::Always).value();
        // Force reset time to be in the past
        always_zone->set_reset_time(std::chrono::steady_clock::now() - std::chrono::minutes(1));
        REQUIRE(always_zone->needs_reset());
    }

    SECTION("Zone statistics") {
        auto zone = Zone::create(EntityId{100}, "Test Zone").value();

        const auto &stats = zone->stats();
        REQUIRE(stats.reset_count == 0);
        REQUIRE(stats.player_count == 0);

        zone->increment_reset_count();
        REQUIRE(zone->stats().reset_count == 1);

        zone->set_player_count(5);
        REQUIRE(zone->stats().player_count == 5);
    }
}


TEST_CASE("World Utils: Zone Command Parser", "[world][utils][zone][parser]") {
    SECTION("Parse zone command line") {
        std::string line = "M 1 3001 1001 0 5";
        auto result = ZoneCommandParser::parse_command_line(line);

        REQUIRE(result.has_value());
        auto cmd = result.value();

        REQUIRE(cmd.command_type == ZoneCommandType::Load_Mobile);
        REQUIRE(cmd.if_flag == 1);
        REQUIRE(cmd.entity_id == EntityId{3001});
        REQUIRE(cmd.room_id == EntityId{1001});
        REQUIRE(cmd.max_count == 5);
    }

    SECTION("Parse comment lines") {
        std::string comment = "* This is a comment";
        auto result = ZoneCommandParser::parse_command_line(comment);

        REQUIRE(result.has_value());
        auto cmd = result.value();
        REQUIRE(cmd.command_type == ZoneCommandType::Comment);
    }

    SECTION("Format command back to string") {
        ZoneCommand cmd;
        cmd.command_type = ZoneCommandType::Load_Object;
        cmd.if_flag = 2;
        cmd.entity_id = EntityId{2001};
        cmd.room_id = EntityId{1001};
        cmd.max_count = 3;

        std::string formatted = ZoneCommandParser::format_command(cmd);
        REQUIRE(formatted == "O 2 2001 1001 0 3");
    }
}

TEST_CASE("World Utils: Zone Utilities", "[world][utils][zone][utils]") {
    SECTION("Reset mode utilities") {
        REQUIRE(ZoneUtils::get_reset_mode_name(ResetMode::Never) == "Never");
        REQUIRE(ZoneUtils::get_reset_mode_name(ResetMode::Empty) == "Empty");

        auto mode = ZoneUtils::parse_reset_mode("Always");
        REQUIRE(mode.has_value());
        REQUIRE(mode.value() == ResetMode::Always);
    }

    SECTION("Zone flag utilities") {
        REQUIRE(ZoneUtils::get_flag_name(ZoneFlag::Closed) == "Closed");
        REQUIRE(ZoneUtils::get_flag_name(ZoneFlag::Quest) == "Quest");

        auto flag = ZoneUtils::parse_zone_flag("NoMortals");
        REQUIRE(flag.has_value());
        REQUIRE(flag.value() == ZoneFlag::NoMortals);
    }

    SECTION("Command type utilities") {
        REQUIRE(ZoneUtils::get_command_type_name(ZoneCommandType::Load_Mobile) == "Load_Mobile");

        auto type = ZoneUtils::parse_command_type("Load_Object");
        REQUIRE(type.has_value());
        REQUIRE(type.value() == ZoneCommandType::Load_Object);

        auto char_type = ZoneUtils::get_command_type_from_char('M');
        REQUIRE(char_type.has_value());
        REQUIRE(char_type.value() == ZoneCommandType::Load_Mobile);

        char cmd_char = ZoneUtils::get_char_from_command_type(ZoneCommandType::Load_Object);
        REQUIRE(cmd_char == 'O');
    }
}

TEST_CASE("World Utils: Room Utilities", "[world][utils][room][utils]") {
    SECTION("Sector utilities") {
        REQUIRE(RoomUtils::get_sector_name(SectorType::Forest) == "Forest");
        REQUIRE(RoomUtils::get_sector_name(SectorType::Water_Swim) == "Water_Swim");

        auto sector = RoomUtils::parse_sector_type("Desert");
        REQUIRE(sector.has_value());
        REQUIRE(sector.value() == SectorType::Desert);
    }

    SECTION("Movement cost calculations") {
        REQUIRE(RoomUtils::get_movement_cost(SectorType::Road) == 1);
        REQUIRE(RoomUtils::get_movement_cost(SectorType::Mountains) == 4);
        REQUIRE(RoomUtils::get_movement_cost(SectorType::Swamp) == 4);
    }

    SECTION("Sector properties") {
        REQUIRE(RoomUtils::is_water_sector(SectorType::Water_Swim));
        REQUIRE(RoomUtils::is_water_sector(SectorType::Underwater));
        REQUIRE_FALSE(RoomUtils::is_water_sector(SectorType::Forest));

        REQUIRE(RoomUtils::requires_swimming(SectorType::Water_Swim));
        REQUIRE_FALSE(RoomUtils::requires_swimming(SectorType::Water_Noswim));

        REQUIRE(RoomUtils::sector_allows_flying(SectorType::Field));
        REQUIRE_FALSE(RoomUtils::sector_allows_flying(SectorType::Underwater));
    }

    SECTION("Light level calculations") {
        REQUIRE(RoomUtils::get_sector_light_level(SectorType::Inside) == 0);
        REQUIRE(RoomUtils::get_sector_light_level(SectorType::Desert) == 4);
        REQUIRE(RoomUtils::get_sector_light_level(SectorType::Fire) == 6);
    }
}

// Note: WorldManager tests would require more complex setup with temporary directories
// and would be integration tests rather than unit tests. They are omitted here
// to keep the tests focused on unit-level functionality.

TEST_CASE("World Utils: WorldManager", "[world][utils][manager]") {
    SECTION("Singleton access") {
        auto &world1 = WorldManager::instance();
        auto &world2 = WorldManager::instance();

        REQUIRE(&world1 == &world2);
    }

    // Note: Full WorldManager testing would require:
    // - Temporary test directories
    // - Mock file systems
    // - Integration with actual Room/Zone objects
    // These would be better suited for integration tests
}

TEST_CASE("World Utils: Object JSON Parsing", "[world][utils][object][json]") {
    SECTION("Object parsing preserves exact short_description from JSON") {
        // Create JSON with specific short_description that differs from name_list
        nlohmann::json object_json = {
            {"id", "3007"},
            {"type", "TOUCHSTONE"},
            {"name_list", "sculpture mielikki goddess"},
            {"short_description", "the living sculpture of Mielikki"}
        };

        // Parse the object from JSON
        auto object_result = Object::from_json(object_json);
        REQUIRE(object_result.has_value());

        auto object = std::move(object_result.value());

        // Verify the object was created correctly
        REQUIRE(object->id() == EntityId{3007});
        REQUIRE(object->name() == "sculpture mielikki goddess");

        // This is the critical test: short_description should be the exact value from JSON,
        // not the fallback to name_list/keywords
        REQUIRE(object->short_description() == "the living sculpture of Mielikki");
        REQUIRE(object->short_description() != object->name()); // Ensure it's not falling back to name
    }

    SECTION("Object parsing handles empty short_description correctly") {
        // Create JSON without short_description field
        nlohmann::json object_json = {
            {"id", "3008"},
            {"type", "OTHER"},
            {"name_list", "test object thing"}
        };

        // Parse the object from JSON
        auto object_result = Object::from_json(object_json);
        REQUIRE(object_result.has_value());

        auto object = std::move(object_result.value());

        // When no short_description is provided, it should fall back to name
        REQUIRE(object->short_description() == "test object thing");
        REQUIRE(object->short_description() == object->name());
    }

    SECTION("Object parsing handles blank short_description correctly") {
        // Create JSON with empty string short_description
        nlohmann::json object_json = {
            {"id", "3009"},
            {"type", "OTHER"},
            {"name_list", "blank description item"},
            {"short_description", ""}
        };

        // Parse the object from JSON
        auto object_result = Object::from_json(object_json);
        REQUIRE(object_result.has_value());

        auto object = std::move(object_result.value());

        // Empty short_description should be preserved as empty (the getter will handle fallback)
        REQUIRE(object->short_description() == "blank description item"); // Fallback behavior from getter
    }
}
