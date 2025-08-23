/***************************************************************************
 *   File: tests/test_world_management.cpp                Part of FieryMUD *
 *  Usage: Unit tests for world management components                      *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.       *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 ***************************************************************************/

#include "world/room.hpp"
#include "world/world_manager.hpp"
#include "world/zone.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <fstream>

TEST_CASE("Direction utilities", "[world][room][direction]") {
    SECTION("Direction name conversion") {
        REQUIRE(RoomUtils::get_direction_name(Direction::North) == "North");
        REQUIRE(RoomUtils::get_direction_name(Direction::Southeast) == "Southeast");
        REQUIRE(RoomUtils::get_direction_name(Direction::None) == "None");
    }

    SECTION("Direction parsing") {
        auto north = RoomUtils::parse_direction("north");
        REQUIRE(north.has_value());
        REQUIRE(north.value() == Direction::North);

        auto n = RoomUtils::parse_direction("n");
        REQUIRE(n.has_value());
        REQUIRE(n.value() == Direction::North);

        auto invalid = RoomUtils::parse_direction("invalid");
        REQUIRE_FALSE(invalid.has_value());
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

TEST_CASE("ExitInfo functionality", "[world][room][exit]") {
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

TEST_CASE("Room basic functionality", "[world][room]") {
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

    SECTION("Room flag management") {
        auto room = Room::create(EntityId{1001}, "Test Room", SectorType::Inside).value();

        REQUIRE_FALSE(room->has_flag(RoomFlag::Dark));

        room->set_flag(RoomFlag::Dark);
        REQUIRE(room->has_flag(RoomFlag::Dark));

        room->remove_flag(RoomFlag::Dark);
        REQUIRE_FALSE(room->has_flag(RoomFlag::Dark));
    }

    SECTION("Room state queries") {
        auto room = Room::create(EntityId{1001}, "Test Room", SectorType::Inside).value();

        REQUIRE_FALSE(room->is_peaceful());
        room->set_flag(RoomFlag::Peaceful);
        REQUIRE(room->is_peaceful());

        REQUIRE(room->allows_magic());
        room->set_flag(RoomFlag::NoMagic);
        REQUIRE_FALSE(room->allows_magic());
    }
}

TEST_CASE("Room exit system", "[world][room][exits]") {
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

TEST_CASE("Room lighting system", "[world][room][lighting]") {
    SECTION("Natural lighting") {
        auto outdoor_room = Room::create(EntityId{1001}, "Outdoor", SectorType::Field).value();
        auto indoor_room = Room::create(EntityId{1002}, "Indoor", SectorType::Inside).value();

        REQUIRE(outdoor_room->is_naturally_lit());
        REQUIRE_FALSE(indoor_room->is_naturally_lit());

        // Dark flag overrides natural light
        outdoor_room->set_flag(RoomFlag::Dark);
        REQUIRE_FALSE(outdoor_room->is_naturally_lit());
    }

    SECTION("Light level calculations") {
        auto room = Room::create(EntityId{1001}, "Test Room", SectorType::Inside).value();

        // Indoor room should have low light initially
        int initial_light = room->calculate_effective_light();

        // Set manual light level
        room->set_light_level(10);
        REQUIRE(room->calculate_effective_light() >= initial_light + 10);

        // Dark flag should force light to 0
        room->set_flag(RoomFlag::Dark);
        REQUIRE(room->calculate_effective_light() == 0);
    }
}

TEST_CASE("Room capacity and occupancy", "[world][room][capacity]") {
    SECTION("Room capacity limits") {
        auto normal_room = Room::create(EntityId{1001}, "Normal Room", SectorType::Inside).value();
        auto tunnel = Room::create(EntityId{1002}, "Tunnel", SectorType::Inside).value();
        auto private_room = Room::create(EntityId{1003}, "Private Room", SectorType::Inside).value();

        tunnel->set_flag(RoomFlag::Tunnel);
        private_room->set_flag(RoomFlag::Private);

        REQUIRE(normal_room->max_occupants() == 100);
        REQUIRE(tunnel->max_occupants() == 1);
        REQUIRE(private_room->max_occupants() == 2);
    }
}

TEST_CASE("Room JSON serialization", "[world][room][json]") {
    SECTION("Complete room serialization") {
        auto room = Room::create(EntityId{1001}, "Test Room", SectorType::Forest).value();
        room->set_description("A dense forest with tall trees.");
        std::vector<std::string> keywords = {"forest", "trees", "dense"};
        room->set_keywords(keywords);
        room->set_light_level(5);
        room->set_zone_id(EntityId{100});
        room->set_flag(RoomFlag::NoMagic);
        room->set_flag(RoomFlag::Peaceful);

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
        REQUIRE(json["light_level"] == 5);
        REQUIRE(json["zone_id"] == 100);

        REQUIRE(json["flags"].is_array());
        REQUIRE(json["flags"].size() == 2);

        REQUIRE(json["exits"].contains("North"));
        REQUIRE(json["exits"]["North"]["to_room"] == 1002);

        // Test round-trip
        auto restored_result = Room::from_json(json);
        REQUIRE(restored_result.has_value());

        auto restored = std::move(restored_result.value());
        REQUIRE(restored->id() == room->id());
        REQUIRE(restored->name() == room->name());
        REQUIRE(restored->sector_type() == room->sector_type());
        REQUIRE(restored->light_level() == room->light_level());
        REQUIRE(restored->zone_id() == room->zone_id());
        REQUIRE(restored->has_flag(RoomFlag::NoMagic));
        REQUIRE(restored->has_flag(RoomFlag::Peaceful));
        REQUIRE(restored->has_exit(Direction::North));
    }
}

TEST_CASE("Zone basic functionality", "[world][zone]") {
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

TEST_CASE("Zone flag management", "[world][zone][flags]") {
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

TEST_CASE("Zone room management", "[world][zone][rooms]") {
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

TEST_CASE("Zone command system", "[world][zone][commands]") {
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

    SECTION("Zone command JSON serialization") {
        ZoneCommand cmd;
        cmd.command_type = ZoneCommandType::Load_Object;
        cmd.if_flag = 2;
        cmd.entity_id = EntityId{2001};
        cmd.room_id = EntityId{1001};
        cmd.comment = "O 2 2001 1001 0 0";

        auto json = cmd.to_json();
        REQUIRE(json["command_type"] == "Load_Object");
        REQUIRE(json["if_flag"] == 2);
        REQUIRE(json["entity_id"] == 2001);
        REQUIRE(json["room_id"] == 1001);

        auto restored_result = ZoneCommand::from_json(json);
        REQUIRE(restored_result.has_value());

        auto restored = restored_result.value();
        REQUIRE(restored.command_type == cmd.command_type);
        REQUIRE(restored.if_flag == cmd.if_flag);
        REQUIRE(restored.entity_id == cmd.entity_id);
        REQUIRE(restored.room_id == cmd.room_id);
    }
}

TEST_CASE("Zone reset system", "[world][zone][reset]") {
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

TEST_CASE("Zone JSON serialization", "[world][zone][json]") {
    SECTION("Complete zone serialization") {
        auto zone = Zone::create(EntityId{100}, "Test Zone", 45, ResetMode::Empty).value();
        zone->set_description("A test zone for unit testing.");
        std::vector<std::string> keywords = {"test", "zone", "unit"};
        zone->set_keywords(keywords);
        zone->set_min_level(10);
        zone->set_max_level(50);
        zone->set_builders("TestBuilder");
        zone->set_first_room(EntityId{1001});
        zone->set_last_room(EntityId{1099});
        zone->set_flag(ZoneFlag::Quest);
        zone->set_flag(ZoneFlag::Recall_Ok);

        zone->add_room(EntityId{1001});
        zone->add_room(EntityId{1002});

        ZoneCommand cmd;
        cmd.command_type = ZoneCommandType::Load_Mobile;
        cmd.if_flag = 1;
        cmd.entity_id = EntityId{3001};
        cmd.room_id = EntityId{1001};
        zone->add_command(cmd);

        auto json = zone->to_json();

        REQUIRE(json["id"] == 100);
        REQUIRE(json["name"] == "Test Zone");
        REQUIRE(json["reset_minutes"] == 45);
        REQUIRE(json["reset_mode"] == "Empty");
        REQUIRE(json["min_level"] == 10);
        REQUIRE(json["max_level"] == 50);
        REQUIRE(json["builders"] == "TestBuilder");
        REQUIRE(json["first_room"] == 1001);
        REQUIRE(json["last_room"] == 1099);

        REQUIRE(json["flags"].is_array());
        REQUIRE(json["flags"].size() == 2);

        REQUIRE(json["rooms"].is_array());
        REQUIRE(json["rooms"].size() == 2);

        REQUIRE(json["commands"].is_array());
        REQUIRE(json["commands"].size() == 1);

        // Test round-trip
        auto restored_result = Zone::from_json(json);
        REQUIRE(restored_result.has_value());

        auto restored = std::move(restored_result.value());
        REQUIRE(restored->id() == zone->id());
        REQUIRE(restored->name() == zone->name());
        REQUIRE(restored->reset_minutes() == zone->reset_minutes());
        REQUIRE(restored->reset_mode() == zone->reset_mode());
        REQUIRE(restored->min_level() == zone->min_level());
        REQUIRE(restored->max_level() == zone->max_level());
        REQUIRE(restored->builders() == zone->builders());
        REQUIRE(restored->has_flag(ZoneFlag::Quest));
        REQUIRE(restored->has_flag(ZoneFlag::Recall_Ok));
        REQUIRE(restored->contains_room(EntityId{1001}));
        REQUIRE(restored->contains_room(EntityId{1002}));
        REQUIRE(restored->commands().size() == 1);
    }
}

TEST_CASE("Zone command parser", "[world][zone][parser]") {
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

TEST_CASE("Zone utility functions", "[world][zone][utils]") {
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

TEST_CASE("Room utility functions", "[world][room][utils]") {
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

TEST_CASE("WorldManager basic functionality", "[world][manager]") {
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