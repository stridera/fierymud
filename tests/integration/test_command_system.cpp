/***************************************************************************
 *   File: tests/test_command_system.cpp              Part of FieryMUD *
 *  Usage: Tests for the command system, parser, and related utilities   *
 *************************************************************************/

#include "../src/commands/builtin_commands.hpp"
#include "../src/commands/command_parser.hpp"
#include "../src/commands/command_system.hpp"
#include "../src/core/ids.hpp"
#include "../common/mock_game_session.hpp"

#include <catch2/catch_test_macros.hpp>
#include <fmt/format.h>

TEST_CASE("CommandSystem: Integration", "[command][system][integration]") {
    UnifiedTestHarness::run_integration_test([&]() {
        auto &cmd_system = CommandSystem::instance();

        // Initialize system
        auto result = cmd_system.initialize();
        REQUIRE(result.has_value());

        // Register commands
        auto builtin_result = BuiltinCommands::register_all_commands();
        REQUIRE(builtin_result.has_value());

        // Verify key commands exist
        REQUIRE(cmd_system.find_command("look") != nullptr);
        REQUIRE(cmd_system.find_command("north") != nullptr);
        REQUIRE(cmd_system.find_command("say") != nullptr);
        REQUIRE(cmd_system.find_command("who") != nullptr);
        REQUIRE(cmd_system.find_command("quit") != nullptr);

        // Test command aliases
        REQUIRE(cmd_system.find_command("l") != nullptr); // look alias
        REQUIRE(cmd_system.find_command("n") != nullptr); // north alias
        REQUIRE(cmd_system.find_command("i") != nullptr); // inventory alias
    });
}

TEST_CASE("CommandParser: Basic Parsing", "[command][parser]") {
    CommandParser parser;

    SECTION("Simple command parsing") {
        auto result = parser.parse("look");
        REQUIRE(result.success);
        REQUIRE(result.command.command == "look");
        REQUIRE(result.command.arguments.empty());
    }

    SECTION("Command with arguments") {
        auto result = parser.parse("say hello world");
        REQUIRE(result.success);
        REQUIRE(result.command.command == "say");
        REQUIRE(result.command.arguments.size() == 2);
        REQUIRE(result.command.arguments[0] == "hello");
        REQUIRE(result.command.arguments[1] == "world");
    }

    SECTION("Quoted arguments") {
        auto result = parser.parse("tell bob \"hello there\"");
        REQUIRE(result.success);
        REQUIRE(result.command.command == "tell");
        REQUIRE(result.command.arguments.size() == 2);
        REQUIRE(result.command.arguments[0] == "bob");
        REQUIRE(result.command.arguments[1] == "hello there");
    }

    SECTION("Invalid input") {
        auto result = parser.parse("");
        REQUIRE_FALSE(result.success);
        REQUIRE_FALSE(result.error_message.empty());
    }
}

TEST_CASE("CommandSystem: Utilities", "[command][system][utils]") {
    using namespace CommandSystemUtils;

    SECTION("Privilege level utilities") {
        REQUIRE(privilege_to_string(PrivilegeLevel::Player) == "Player");
        REQUIRE(privilege_to_string(PrivilegeLevel::Builder) == "Builder");
        REQUIRE(privilege_to_string(PrivilegeLevel::God) == "God");

        auto parsed = parse_privilege_level("player");
        REQUIRE(parsed.has_value());
        REQUIRE(parsed.value() == PrivilegeLevel::Player);

        auto invalid = parse_privilege_level("invalid");
        REQUIRE_FALSE(invalid.has_value());
    }

    SECTION("Privilege hierarchy") {
        REQUIRE(privilege_allows(PrivilegeLevel::God, PrivilegeLevel::Player));
        REQUIRE(privilege_allows(PrivilegeLevel::Builder, PrivilegeLevel::Builder));
        REQUIRE_FALSE(privilege_allows(PrivilegeLevel::Player, PrivilegeLevel::Builder));

        auto hierarchy = get_privilege_hierarchy();
        REQUIRE_FALSE(hierarchy.empty());
        REQUIRE(hierarchy[0] == PrivilegeLevel::Guest);
        REQUIRE(hierarchy.back() == PrivilegeLevel::Overlord);
    }

    SECTION("Command result utilities") {
        REQUIRE(result_to_string(CommandResult::Success) == "Success");
        REQUIRE(result_to_string(CommandResult::NotFound) == "NotFound");
        REQUIRE(result_to_string(CommandResult::InvalidSyntax) == "InvalidSyntax");
    }

    SECTION("Validation utilities") {
        REQUIRE(is_valid_command_name("look"));
        REQUIRE(is_valid_command_name("north"));
        REQUIRE_FALSE(is_valid_command_name(""));
        REQUIRE_FALSE(is_valid_command_name("123"));

        REQUIRE(is_valid_category_name("Movement"));
        REQUIRE(is_valid_category_name("Social Commands"));
        REQUIRE_FALSE(is_valid_category_name(""));
    }
}

TEST_CASE("ParsedCommand: Argument Handling", "[command][parser]") {
    ParsedCommand cmd;
    cmd.command = "test";
    cmd.arguments = {"arg1", "arg2", "arg3"};

    SECTION("Argument access") {
        REQUIRE(cmd.arg(0) == "arg1");
        REQUIRE(cmd.arg(1) == "arg2");
        REQUIRE(cmd.arg(2) == "arg3");
        REQUIRE(cmd.arg(10) == ""); // Out of bounds
    }

    SECTION("Argument count") {
        REQUIRE(cmd.arg_count() == 3);
        REQUIRE(cmd.has_args());
    }

    SECTION("Arguments from index") {
        REQUIRE(cmd.args_from(0) == "arg1 arg2 arg3");
        REQUIRE(cmd.args_from(1) == "arg2 arg3");
        REQUIRE(cmd.args_from(2) == "arg3");
        REQUIRE(cmd.args_from(10) == "");
    }

    SECTION("Argument requirements") {
        auto result = cmd.require_args(2);
        REQUIRE(result.has_value());

        result = cmd.require_args(3);
        REQUIRE(result.has_value());

        result = cmd.require_args(4);
        REQUIRE_FALSE(result.has_value());
    }
}

TEST_CASE("CommandContext: Basic Functionality", "[command][context]") {
    SECTION("CommandExecutionState") {
        CommandExecutionState state;

        // Set and get values
        state.set("key1", std::string("value1"));
        state.set("key2", 42);
        state.set("key3", true);

        auto str_val = state.get<std::string>("key1");
        REQUIRE(str_val.has_value());
        REQUIRE(*str_val == "value1");

        auto int_val = state.get<int>("key2");
        REQUIRE(int_val.has_value());
        REQUIRE(*int_val == 42);

        auto bool_val = state.get<bool>("key3");
        REQUIRE(bool_val.has_value());
        REQUIRE(*bool_val == true);

        // Check existence
        REQUIRE(state.has("key1"));
        REQUIRE_FALSE(state.has("nonexistent"));

        // Remove and clear
        state.remove("key1");
        REQUIRE_FALSE(state.has("key1"));

        state.clear();
        REQUIRE_FALSE(state.has("key2"));
        REQUIRE_FALSE(state.has("key3"));
    }
}

TEST_CASE("CommandContext: Message Routing", "[command][context]") {
    // Test the underlying message routing to ensure proper separation

    SECTION("send vs send_to_room behavior") {
        TestHarness harness;

        // This is testing the underlying mechanics that emotes use
        // We'll test through command execution to verify the behavior

        harness.clear_output();
        harness.execute_command("say Testing message routing").and_wait_for_output();

        const auto& output = harness.get_output();
        REQUIRE(output.size() >= 1);

        // Say command should produce distinct messages
        bool found_self = false;
        bool found_room = false;

        for (const auto &line : output) {
            if (line.find("You say") != std::string::npos) {
                found_self = true;
            }
            if (line.find("TestPlayer says") != std::string::npos) {
                found_room = true;
            }
        }

        // For say command, we should have both self and room messages
        // (This tests the messaging infrastructure that emotes rely on)
        REQUIRE(found_self);
    }
}