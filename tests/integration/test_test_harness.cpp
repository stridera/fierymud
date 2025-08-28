/***************************************************************************
 *   File: tests/test_test_harness.cpp                Part of FieryMUD *
 *  Usage: Tests for the test harness itself                           *
 ***************************************************************************/

#include "../common/test_harness.hpp"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("TestHarness: Basic Usage", "[harness]") {
    TestHarness harness;

    SECTION("Fluent assertions and fixtures") {
        auto& room = harness.fixtures().create_room("The Fixture Room");
        harness.fixtures().create_npc("A fixture NPC", room);

        harness.execute_command(fmt::format("goto {}", room.id().value()))
               .and_wait_for_output();

        harness.clear_output();
        harness.execute_command("look")
               .and_wait_for_output();

        // Verify we got some output from the look command
        REQUIRE(harness.get_output().size() > 0);
    }
}