#include <catch2/catch_test_macros.hpp>

#include "../../src/text/text_format.hpp"

TEST_CASE("TextFormat: Color Processing", "[text][colors][unit]") {
    SECTION("Basic color tags work") {
        std::string input = "<red>Hello</>";
        std::string result = TextFormat::apply_colors(input);

        // Should contain the red ANSI code
        REQUIRE(result.find("\033[31m") != std::string::npos);
        // Should contain the reset code
        REQUIRE(result.find("\033[0m") != std::string::npos);
        // Should contain the text
        REQUIRE(result.find("Hello") != std::string::npos);
    }

    SECTION("Nested color tags restore previous color") {
        // Input: green text, then brown item, then green text again
        std::string input = "<green>You drink from a <brown>leather skin</>. Refreshing!</>";
        std::string result = TextFormat::apply_colors(input);

        // Should have green code at start
        REQUIRE(result.find("\033[32m") != std::string::npos);
        // Should have brown/yellow code for item (brown maps to yellow in standard ANSI)
        REQUIRE(result.find("\033[33m") != std::string::npos);

        // After the brown close tag, should have reset followed by green restoration
        // The pattern should be: reset + green code after "leather skin"
        size_t brown_close_pos = result.find("leather skin");
        REQUIRE(brown_close_pos != std::string::npos);

        // Find the reset after the brown text
        size_t reset_pos = result.find("\033[0m", brown_close_pos);
        REQUIRE(reset_pos != std::string::npos);

        // After the reset, should have green restored
        size_t green_restore_pos = result.find("\033[32m", reset_pos);
        REQUIRE(green_restore_pos != std::string::npos);

        // The green restoration should come before "Refreshing"
        size_t refreshing_pos = result.find("Refreshing");
        REQUIRE(refreshing_pos != std::string::npos);
        REQUIRE(green_restore_pos < refreshing_pos);
    }

    SECTION("Multiple levels of nesting work correctly") {
        std::string input = "<red>A <green>B <blue>C</> D</> E</>";
        std::string result = TextFormat::apply_colors(input);

        // Pattern should be:
        // red "A " green "B " blue "C" reset green "D" reset red "E" reset

        // All colors should be present
        REQUIRE(result.find("\033[31m") != std::string::npos); // red
        REQUIRE(result.find("\033[32m") != std::string::npos); // green
        REQUIRE(result.find("\033[34m") != std::string::npos); // blue

        // Should have multiple resets (at least 3)
        size_t reset_count = 0;
        size_t pos = 0;
        while ((pos = result.find("\033[0m", pos)) != std::string::npos) {
            reset_count++;
            pos++;
        }
        REQUIRE(reset_count >= 3);
    }

    SECTION("No color tags leaves text unchanged") {
        std::string input = "Plain text without any formatting.";
        std::string result = TextFormat::apply_colors(input);

        // Should be identical to input
        REQUIRE(result == input);
    }

    SECTION("Unclosed tags at end result in reset") {
        std::string input = "<red>Text without closing tag";
        std::string result = TextFormat::apply_colors(input);

        // Should still have the red code
        REQUIRE(result.find("\033[31m") != std::string::npos);
        // Should have a reset at the end due to unclosed tag
        REQUIRE(result.find("\033[0m") != std::string::npos);
    }

    SECTION("Hex colors work") {
        std::string input = "<#FF0000>Red text</>";
        std::string result = TextFormat::apply_colors(input);

        // Should have RGB ANSI code
        REQUIRE(result.find("\033[38;2;255;0;0m") != std::string::npos);
    }

    SECTION("Indexed colors work") {
        std::string input = "<c196>Indexed color text</>";
        std::string result = TextFormat::apply_colors(input);

        // Should have indexed color code
        REQUIRE(result.find("\033[38;5;196m") != std::string::npos);
    }
}

TEST_CASE("TextFormat: Color Stripping", "[text][colors][unit]") {
    SECTION("Strip basic color tags") {
        std::string input = "<red>Hello</> World";
        std::string result = TextFormat::strip_colors(input);

        REQUIRE(result == "Hello World");
    }

    SECTION("Strip nested color tags") {
        std::string input = "<green>Outer <red>Inner</> Back</>";
        std::string result = TextFormat::strip_colors(input);

        REQUIRE(result == "Outer Inner Back");
    }
}

TEST_CASE("TextFormat: Has Colors Check", "[text][colors][unit]") {
    SECTION("Detects color tags") {
        REQUIRE(TextFormat::has_colors("<red>text</>"));
        REQUIRE(TextFormat::has_colors("prefix <blue>text</> suffix"));
    }

    SECTION("Returns false for plain text") {
        REQUIRE_FALSE(TextFormat::has_colors("plain text"));
        REQUIRE_FALSE(TextFormat::has_colors("text with < angle > brackets"));
    }
}
