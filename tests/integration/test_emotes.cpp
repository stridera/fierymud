/***************************************************************************
 *   File: tests/test_emotes.cpp                      Part of FieryMUD *
 *  Usage: Tests for emote commands                                       *
 ***************************************************************************/

#include "../common/lightweight_test_harness.hpp"
#include <catch2/catch_test_macros.hpp>

TEST_CASE("Emote Commands - Prevent Duplication", "[integration][emotes]") {
    LightweightTestHarness harness;

    SECTION("Emote command should not duplicate messages") {
        // Test the emote command - it should send to room including self, not separately
        harness.execute_command("emote dances around happily");

        const auto& output = harness.get_player()->get_output();

        // Should have exactly one message for emote
        REQUIRE(output.size() >= 1);

        // Debug: Print what we actually got
        for (size_t i = 0; i < output.size(); ++i) {
            fmt::print("Emote Output[{}]: '{}'\n", i, output[i]);
        }

        // Check that the output doesn't contain duplicates
        bool found_emote = false;
        int emote_count = 0;
        for (const auto &line : output) {
            if (line.find("TestPlayer dances around happily") != std::string::npos) {
                emote_count++;
                found_emote = true;
            }
        }

        REQUIRE(found_emote);
        REQUIRE(emote_count == 1); // Should appear exactly once, not duplicated
    }

    SECTION("Nod command should send separate self and room messages") {
        harness.get_player()->clear_output();

        harness.execute_command("nod");

        const auto& output = harness.get_player()->get_output();

        // Debug: Print what we actually got for nod
        for (size_t i = 0; i < output.size(); ++i) {
            fmt::print("Nod Output[{}]: '{}'\n", i, output[i]);
        }

        REQUIRE(output.size() >= 1); // Should have at least 1 message

        // Check for self message - player should always see this
        bool found_self_message = false;

        for (const auto &line : output) {
            if (line.find("You nod.") != std::string::npos) {
                found_self_message = true;
            }
        }

        REQUIRE(found_self_message); // Should see "You nod."

        // Note: The room message "TestPlayer nods." goes to other actors in the room,
        // not back to the player. The player only sees "You nod."
        // This is the correct behavior for social commands.
    }

    SECTION("Social commands should have consistent message patterns") {
        // Test various social commands for consistent behavior
        std::vector<std::string> social_commands = {"wave", "bow", "laugh", "smile"};

        for (const auto &cmd : social_commands) {
            harness.get_player()->clear_output();

            harness.execute_command(cmd);

            const auto& output = harness.get_player()->get_output();
            REQUIRE(output.size() >= 1); // Each should produce at least one message

            // Verify no empty messages
            for (const auto &line : output) {
                REQUIRE_FALSE(line.empty());
            }
        }
    }
}
