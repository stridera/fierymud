#include <catch2/catch_test_macros.hpp>
#include "arguments.hpp"

TEST_CASE("Arguments basic functionality", "[arguments]") {
    SECTION("Construction and get()") {
        Arguments args1("hello world test");
        REQUIRE(args1.get() == "hello world test");
        
        Arguments args2{"another test"};
        REQUIRE(args2.get() == "another test");
        
        Arguments args3("");
        REQUIRE(args3.get() == "");
        REQUIRE(args3.empty());
    }
    
    SECTION("Trimming whitespace") {
        Arguments args("  hello world  ");
        REQUIRE(args.get() == "hello world");
        REQUIRE_FALSE(args.empty());
        
        Arguments args_spaces("   ");
        REQUIRE(args_spaces.get() == "");
        REQUIRE(args_spaces.empty());
    }
}

TEST_CASE("Arguments shift() functionality", "[arguments]") {
    SECTION("Basic word shifting") {
        Arguments args("hello world test");
        
        auto word1 = args.shift();
        REQUIRE(word1 == "hello");
        REQUIRE(args.get() == "world test");
        
        auto word2 = args.shift();
        REQUIRE(word2 == "world");
        REQUIRE(args.get() == "test");
        
        auto word3 = args.shift();
        REQUIRE(word3 == "test");
        REQUIRE(args.get() == "");
        REQUIRE(args.empty());
        
        auto word4 = args.shift();
        REQUIRE(word4 == "");
    }
    
    SECTION("Quoted string handling") {
        Arguments args("\"hello world\" test 'single quoted'");
        
        auto quoted1 = args.shift();
        REQUIRE(quoted1 == "hello world");
        REQUIRE(args.get() == "test 'single quoted'");
        
        auto word = args.shift();
        REQUIRE(word == "test");
        
        auto quoted2 = args.shift();
        REQUIRE(quoted2 == "single quoted");
    }
    
    SECTION("Unclosed quotes") {
        Arguments args("\"unclosed quote test");
        auto result = args.shift();
        REQUIRE(result == "unclosed quote test");
        REQUIRE(args.empty());
    }
}

TEST_CASE("Arguments command_shift() functionality", "[arguments]") {
    SECTION("Non-alpha character extraction") {
        Arguments args(";hello world");
        auto cmd = args.command_shift();
        REQUIRE(cmd == ";");
        REQUIRE(args.get() == "hello world");
        
        Arguments args2(".test command");
        auto cmd2 = args2.command_shift();
        REQUIRE(cmd2 == ".");
        REQUIRE(args2.get() == "test command");
    }
    
    SECTION("Alpha character - non-strict mode") {
        Arguments args("hello world");
        auto cmd = args.command_shift(false);
        REQUIRE(cmd == "hello");
        REQUIRE(args.get() == "world");
    }
    
    SECTION("Alpha character - strict mode") {
        Arguments args("hello world");
        auto cmd = args.command_shift(true);
        REQUIRE(cmd == "");
        REQUIRE(args.get() == "hello world"); // Should not consume
    }
}

TEST_CASE("Arguments try_shift_number() - NEW BEHAVIOR", "[arguments]") {
    SECTION("Successful number parsing - should consume") {
        Arguments args("123 hello world");
        auto result = args.try_shift_number();
        
        REQUIRE(result.has_value());
        REQUIRE(result.value() == 123);
        REQUIRE(args.get() == "hello world"); // Should be consumed
    }
    
    SECTION("Failed parsing - should NOT consume") {
        Arguments args("hello 123 world");
        auto result = args.try_shift_number();
        
        REQUIRE_FALSE(result.has_value());
        REQUIRE(args.get() == "hello 123 world"); // Should NOT be consumed
    }
    
    SECTION("Empty arguments - should NOT consume") {
        Arguments args("");
        auto result = args.try_shift_number();
        
        REQUIRE_FALSE(result.has_value());
        REQUIRE(args.get() == "");
        REQUIRE(args.empty());
    }
    
    SECTION("Whitespace only - should NOT consume") {
        Arguments args("   ");
        auto result = args.try_shift_number();
        
        REQUIRE_FALSE(result.has_value());
        REQUIRE(args.get() == "");
    }
    
    SECTION("Zero - should consume") {
        Arguments args("0 test");
        auto result = args.try_shift_number();
        
        REQUIRE(result.has_value());
        REQUIRE(result.value() == 0);
        REQUIRE(args.get() == "test");
    }
    
    SECTION("Negative number - behavior depends on is_integer implementation") {
        Arguments args("-123 test");
        auto result = args.try_shift_number();
        
        // The behavior here depends on how is_integer handles negative numbers
        // If it rejects them, the argument should not be consumed
        if (!result.has_value()) {
            REQUIRE(args.get() == "-123 test"); // Should not be consumed
        } else {
            REQUIRE(result.value() == -123);
            REQUIRE(args.get() == "test"); // Should be consumed
        }
    }
    
    SECTION("Partial number - should NOT consume") {
        Arguments args("123abc hello");
        auto result = args.try_shift_number();
        
        REQUIRE_FALSE(result.has_value());
        REQUIRE(args.get() == "123abc hello"); // Should NOT be consumed
    }
}

TEST_CASE("Arguments try_shift_number_and_arg() - NEW BEHAVIOR", "[arguments]") {
    SECTION("Valid number.item format - should consume") {
        Arguments args("3.sword hello world");
        auto result = args.try_shift_number_and_arg();
        
        REQUIRE(result.has_value());
        REQUIRE(result->first == 3);
        REQUIRE(result->second == "sword");
        REQUIRE(args.get() == "hello world"); // Should be consumed
    }
    
    SECTION("'all.item' format - should consume") {
        Arguments args("all.potion rest");
        auto result = args.try_shift_number_and_arg();
        
        REQUIRE(result.has_value());
        REQUIRE(result->first == Arguments::MAX_ITEMS);
        REQUIRE(result->second == "potion");
        REQUIRE(args.get() == "rest"); // Should be consumed
    }
    
    SECTION("No dot format - should NOT consume") {
        Arguments args("sword hello world");
        auto result = args.try_shift_number_and_arg();
        
        REQUIRE_FALSE(result.has_value());
        REQUIRE(args.get() == "sword hello world"); // Should NOT be consumed
    }
    
    SECTION("Invalid number format - should NOT consume") {
        Arguments args("abc.sword hello");
        auto result = args.try_shift_number_and_arg();
        
        REQUIRE_FALSE(result.has_value());
        REQUIRE(args.get() == "abc.sword hello"); // Should NOT be consumed
    }
    
    SECTION("Empty number part - should NOT consume") {
        Arguments args(".sword hello");
        auto result = args.try_shift_number_and_arg();
        
        REQUIRE_FALSE(result.has_value());
        REQUIRE(args.get() == ".sword hello"); // Should NOT be consumed
    }
    
    SECTION("Empty item part - should NOT consume") {
        Arguments args("3. hello");
        auto result = args.try_shift_number_and_arg();
        
        REQUIRE_FALSE(result.has_value());
        REQUIRE(args.get() == "3. hello"); // Should NOT be consumed
    }
    
    SECTION("Just a dot - should NOT consume") {
        Arguments args(". hello");
        auto result = args.try_shift_number_and_arg();
        
        REQUIRE_FALSE(result.has_value());
        REQUIRE(args.get() == ". hello"); // Should NOT be consumed
    }
    
    SECTION("Multiple dots - should consume first part") {
        Arguments args("5.item.name hello");
        auto result = args.try_shift_number_and_arg();
        
        REQUIRE(result.has_value());
        REQUIRE(result->first == 5);
        REQUIRE(result->second == "item.name");
        REQUIRE(args.get() == "hello"); // Should be consumed
    }
    
    SECTION("Zero count - should consume") {
        Arguments args("0.sword hello");
        auto result = args.try_shift_number_and_arg();
        
        REQUIRE(result.has_value());
        REQUIRE(result->first == 0);
        REQUIRE(result->second == "sword");
        REQUIRE(args.get() == "hello");
    }
}

TEST_CASE("Arguments shift_clean() functionality", "[arguments]") {
    SECTION("Double dollar replacement") {
        Arguments args("test$$string hello$$world");
        auto result = args.shift_clean();
        REQUIRE(result == "test$string");
        REQUIRE(args.get() == "hello$$world");
        
        auto result2 = args.shift_clean();
        REQUIRE(result2 == "hello$world");
    }
    
    SECTION("No double dollars") {
        Arguments args("normal string");
        auto result = args.shift_clean();
        REQUIRE(result == "normal");
        REQUIRE(args.get() == "string");
    }
}

TEST_CASE("Arguments edge cases", "[arguments]") {
    SECTION("Multiple consecutive spaces") {
        Arguments args("hello    world     test");
        auto word1 = args.shift();
        REQUIRE(word1 == "hello");
        REQUIRE(args.get() == "world     test");
        
        auto word2 = args.shift();
        REQUIRE(word2 == "world");
        REQUIRE(args.get() == "test");
    }
    
    SECTION("Leading and trailing spaces with shifts") {
        Arguments args("  hello  world  ");
        REQUIRE(args.get() == "hello  world");
        
        auto word1 = args.shift();
        REQUIRE(word1 == "hello");
        REQUIRE(args.get() == "world");
        
        auto word2 = args.shift();
        REQUIRE(word2 == "world");
        REQUIRE(args.empty());
    }
    
    SECTION("Only quotes") {
        Arguments args("\"\" ''");
        auto empty1 = args.shift();
        REQUIRE(empty1 == "");
        
        auto empty2 = args.shift();
        REQUIRE(empty2 == "");
        
        REQUIRE(args.empty());
    }
}

TEST_CASE("Arguments state preservation on failed try_shift operations", "[arguments]") {
    SECTION("Multiple failed try_shift_number calls should not affect state") {
        Arguments args("hello world 123");
        
        // First failed attempt
        auto result1 = args.try_shift_number();
        REQUIRE_FALSE(result1.has_value());
        REQUIRE(args.get() == "hello world 123");
        
        // Second failed attempt on same args
        auto result2 = args.try_shift_number();
        REQUIRE_FALSE(result2.has_value());
        REQUIRE(args.get() == "hello world 123");
        
        // Should still be able to shift normally
        auto word = args.shift();
        REQUIRE(word == "hello");
        REQUIRE(args.get() == "world 123");
        
        // Now try_shift_number should still fail
        auto result3 = args.try_shift_number();
        REQUIRE_FALSE(result3.has_value());
        REQUIRE(args.get() == "world 123");
        
        // Skip to the number
        [[maybe_unused]] auto consumed = args.shift(); // consume "world"
        auto result4 = args.try_shift_number();
        REQUIRE(result4.has_value());
        REQUIRE(result4.value() == 123);
        REQUIRE(args.empty());
    }
    
    SECTION("Mixed successful and failed operations") {
        Arguments args("123 hello 456.sword world");
        
        // Successful number shift  
        auto num1 = args.try_shift_number();
        REQUIRE(num1.has_value());
        REQUIRE(num1.value() == 123);
        REQUIRE(args.get() == "hello 456.sword world");
        
        // Failed number shift
        auto num2 = args.try_shift_number();
        REQUIRE_FALSE(num2.has_value());
        REQUIRE(args.get() == "hello 456.sword world");
        
        // Successful regular shift
        auto word = args.shift();
        REQUIRE(word == "hello");
        REQUIRE(args.get() == "456.sword world");
        
        // Successful number_and_arg shift
        auto num_item = args.try_shift_number_and_arg();
        REQUIRE(num_item.has_value());
        REQUIRE(num_item->first == 456);
        REQUIRE(num_item->second == "sword");
        REQUIRE(args.get() == "world");
    }
}