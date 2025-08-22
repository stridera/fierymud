/***************************************************************************
 *   File: tests/test_harness_usage.cpp               Part of FieryMUD *
 *  Usage: Examples and tests demonstrating TestHarness usage             *
 ***************************************************************************/

#include "test_harness.hpp"

TEST_CASE("Test Harness Basic Usage", "[harness]") {
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
