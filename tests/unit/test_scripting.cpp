/***************************************************************************
 *   File: tests/unit/test_scripting.cpp                      Part of FieryMUD *
 *  Usage: Unit tests for Lua scripting system                              *
 ***************************************************************************/

#include "../src/scripting/script_engine.hpp"
#include "../src/scripting/triggers/trigger_types.hpp"
#include "../src/scripting/triggers/trigger_data.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/reporters/catch_reporter_event_listener.hpp>
#include <catch2/reporters/catch_reporter_registrars.hpp>

using namespace FieryMUD;

// Event listener to ensure proper ScriptEngine cleanup before static destruction
class ScriptEngineCleanupListener : public Catch::EventListenerBase {
public:
    using Catch::EventListenerBase::EventListenerBase;

    void testRunEnded(Catch::TestRunStats const&) override {
        auto& engine = ScriptEngine::instance();
        if (engine.is_initialized()) {
            engine.shutdown();
        }
    }
};

CATCH_REGISTER_LISTENER(ScriptEngineCleanupListener)

TEST_CASE("ScriptEngine: Initialization", "[scripting][engine]") {
    auto& engine = ScriptEngine::instance();

    SECTION("Engine can be initialized") {
        auto result = engine.initialize();
        REQUIRE(result.has_value());
        REQUIRE(engine.is_initialized());
    }

    SECTION("Double initialization is safe") {
        // Already initialized from previous section
        auto result = engine.initialize();
        // Should succeed but log a warning
        REQUIRE(result.has_value());
    }
}

TEST_CASE("ScriptEngine: Basic execution", "[scripting][engine]") {
    auto& engine = ScriptEngine::instance();
    if (!engine.is_initialized()) {
        engine.initialize();
    }

    SECTION("Simple arithmetic expression") {
        auto result = engine.execute("return 1 + 2", "test_arith");
        REQUIRE(result.has_value());
        auto& lua_result = result.value();
        REQUIRE(lua_result.get<int>() == 3);
    }

    SECTION("String concatenation") {
        auto result = engine.execute("return 'Hello' .. ' ' .. 'World'", "test_concat");
        REQUIRE(result.has_value());
        auto& lua_result = result.value();
        REQUIRE(lua_result.get<std::string>() == "Hello World");
    }

    SECTION("Table creation") {
        auto result = engine.execute(
            "local t = {a = 1, b = 2}; return t.a + t.b",
            "test_table"
        );
        REQUIRE(result.has_value());
        REQUIRE(result.value().get<int>() == 3);
    }
}

TEST_CASE("ScriptEngine: Utility functions", "[scripting][utils]") {
    auto& engine = ScriptEngine::instance();
    if (!engine.is_initialized()) {
        engine.initialize();
    }

    SECTION("dice() function works") {
        // dice(1, 6) should return 1-6
        auto result = engine.execute(
            "local sum = 0; for i = 1, 100 do sum = sum + dice(1, 6) end; return sum",
            "test_dice"
        );
        REQUIRE(result.has_value());
        int sum = result.value().get<int>();
        // 100 rolls of 1d6, average is 3.5, so sum should be roughly 350
        // With high probability, it's between 150 and 550
        REQUIRE(sum >= 100);  // Minimum possible: 100 * 1 = 100
        REQUIRE(sum <= 600);  // Maximum possible: 100 * 6 = 600
    }

    SECTION("random() function works") {
        auto result = engine.execute(
            "local r = random(10, 20); return r >= 10 and r <= 20",
            "test_random"
        );
        REQUIRE(result.has_value());
        REQUIRE(result.value().get<bool>() == true);
    }

    SECTION("percent_chance() function works") {
        // 100% chance should always succeed
        auto result = engine.execute(
            "return percent_chance(100)",
            "test_percent_100"
        );
        REQUIRE(result.has_value());
        REQUIRE(result.value().get<bool>() == true);

        // 0% chance should always fail
        result = engine.execute(
            "return percent_chance(0)",
            "test_percent_0"
        );
        REQUIRE(result.has_value());
        REQUIRE(result.value().get<bool>() == false);
    }

    SECTION("mud_time() returns table") {
        auto result = engine.execute(
            "local t = mud_time(); return t.hour ~= nil and t.day ~= nil",
            "test_mud_time"
        );
        REQUIRE(result.has_value());
        REQUIRE(result.value().get<bool>() == true);
    }
}

TEST_CASE("ScriptEngine: Sandbox enforcement", "[scripting][sandbox]") {
    auto& engine = ScriptEngine::instance();
    if (!engine.is_initialized()) {
        engine.initialize();
    }

    SECTION("loadfile is blocked") {
        auto result = engine.execute(
            "return loadfile ~= nil",
            "test_loadfile"
        );
        REQUIRE(result.has_value());
        REQUIRE(result.value().get<bool>() == false);
    }

    SECTION("dofile is blocked") {
        auto result = engine.execute(
            "return dofile ~= nil",
            "test_dofile"
        );
        REQUIRE(result.has_value());
        REQUIRE(result.value().get<bool>() == false);
    }

    SECTION("io library is not loaded") {
        auto result = engine.execute(
            "return io == nil",
            "test_io"
        );
        REQUIRE(result.has_value());
        REQUIRE(result.value().get<bool>() == true);
    }

    SECTION("os library is not loaded") {
        auto result = engine.execute(
            "return os == nil",
            "test_os"
        );
        REQUIRE(result.has_value());
        REQUIRE(result.value().get<bool>() == true);
    }

    SECTION("debug library is not loaded") {
        auto result = engine.execute(
            "return debug == nil",
            "test_debug"
        );
        REQUIRE(result.has_value());
        REQUIRE(result.value().get<bool>() == true);
    }
}

TEST_CASE("ScriptEngine: Compilation and caching", "[scripting][compile]") {
    auto& engine = ScriptEngine::instance();
    if (!engine.is_initialized()) {
        engine.initialize();
    }

    SECTION("Valid script compiles successfully") {
        auto result = engine.compile_script(
            "function hello() return 'hello' end",
            "test_valid"
        );
        REQUIRE(result.has_value());
    }

    SECTION("Invalid script fails compilation") {
        auto result = engine.compile_script(
            "function hello( return 'missing paren'",
            "test_invalid"
        );
        REQUIRE_FALSE(result.has_value());
        REQUIRE(result.error() == ScriptError::CompilationFailed);
    }

    SECTION("Syntax error reports correctly") {
        auto result = engine.execute(
            "this is not valid lua syntax!!",
            "test_syntax_error"
        );
        REQUIRE_FALSE(result.has_value());
        REQUIRE(result.error() == ScriptError::ExecutionFailed);
    }
}

TEST_CASE("ScriptEngine: Coroutines", "[scripting][coroutine]") {
    auto& engine = ScriptEngine::instance();
    if (!engine.is_initialized()) {
        engine.initialize();
    }

    SECTION("Coroutine library is available") {
        auto result = engine.execute(
            "return coroutine ~= nil",
            "test_coroutine_lib"
        );
        REQUIRE(result.has_value());
        REQUIRE(result.value().get<bool>() == true);
    }

    SECTION("Can create coroutines") {
        auto result = engine.execute(R"(
            local function count()
                for i = 1, 3 do
                    coroutine.yield(i)
                end
                return 'done'
            end
            local co = coroutine.create(count)
            local status, val1 = coroutine.resume(co)
            local status2, val2 = coroutine.resume(co)
            return val1 + val2
        )", "test_coroutine_create");
        REQUIRE(result.has_value());
        REQUIRE(result.value().get<int>() == 3); // 1 + 2
    }
}

TEST_CASE("TriggerFlag: Bitfield operations", "[scripting][triggers]") {
    SECTION("Single flag has correct value") {
        REQUIRE(static_cast<uint32_t>(TriggerFlag::GLOBAL) == 1);
        REQUIRE(static_cast<uint32_t>(TriggerFlag::RANDOM) == 2);
        REQUIRE(static_cast<uint32_t>(TriggerFlag::COMMAND) == 4);
    }

    SECTION("Flags can be combined") {
        TriggerFlag combined = TriggerFlag::COMMAND | TriggerFlag::SPEECH;
        REQUIRE(has_flag(combined, TriggerFlag::COMMAND));
        REQUIRE(has_flag(combined, TriggerFlag::SPEECH));
        REQUIRE_FALSE(has_flag(combined, TriggerFlag::DEATH));
    }

    SECTION("Multiple flags can be combined and tested") {
        TriggerFlag multi = TriggerFlag::COMMAND | TriggerFlag::SPEECH |
                           TriggerFlag::GREET | TriggerFlag::DEATH;
        REQUIRE(has_flag(multi, TriggerFlag::COMMAND));
        REQUIRE(has_flag(multi, TriggerFlag::SPEECH));
        REQUIRE(has_flag(multi, TriggerFlag::GREET));
        REQUIRE(has_flag(multi, TriggerFlag::DEATH));
        REQUIRE_FALSE(has_flag(multi, TriggerFlag::FIGHT));
        REQUIRE_FALSE(has_flag(multi, TriggerFlag::BRIBE));
    }

    // Note: magic_enum string conversion tests removed because
    // bitfield enums with large values (1 << N) exceed magic_enum's
    // default range. The database uses explicit string mapping instead.
}

TEST_CASE("TriggerData: Structure", "[scripting][triggers]") {
    SECTION("Default construction") {
        TriggerData trigger;
        REQUIRE(trigger.id == 0);
        REQUIRE(trigger.name.empty());
        REQUIRE(trigger.attach_type == ScriptType::MOB);
        REQUIRE_FALSE(trigger.zone_id.has_value());
        REQUIRE_FALSE(trigger.mob_id.has_value());
        REQUIRE_FALSE(trigger.object_id.has_value());
    }

    SECTION("has_flag works correctly") {
        TriggerData trigger;
        trigger.flags = TriggerFlag::COMMAND | TriggerFlag::SPEECH;

        REQUIRE(trigger.has_flag(TriggerFlag::COMMAND));
        REQUIRE(trigger.has_flag(TriggerFlag::SPEECH));
        REQUIRE_FALSE(trigger.has_flag(TriggerFlag::DEATH));
    }

    SECTION("attached_entity_id works for MOB") {
        TriggerData trigger;
        trigger.attach_type = ScriptType::MOB;
        trigger.mob_id = EntityId(30, 42);

        auto entity = trigger.attached_entity_id();
        REQUIRE(entity.has_value());
        REQUIRE(entity->zone_id() == 30);
        REQUIRE(entity->local_id() == 42);
    }

    SECTION("attached_entity_id works for OBJECT") {
        TriggerData trigger;
        trigger.attach_type = ScriptType::OBJECT;
        trigger.object_id = EntityId(50, 10);

        auto entity = trigger.attached_entity_id();
        REQUIRE(entity.has_value());
        REQUIRE(entity->zone_id() == 50);
        REQUIRE(entity->local_id() == 10);
    }

    SECTION("attached_entity_id works for WORLD") {
        TriggerData trigger;
        trigger.attach_type = ScriptType::WORLD;
        trigger.zone_id = 30;

        auto entity = trigger.attached_entity_id();
        REQUIRE(entity.has_value());
        REQUIRE(entity->zone_id() == 30);
        REQUIRE(entity->local_id() == 0);
    }

    SECTION("parse_flags from string array") {
        std::vector<std::string> flag_strings = {"COMMAND", "SPEECH", "GREET"};
        TriggerFlag flags = TriggerData::parse_flags(flag_strings);

        REQUIRE(has_flag(flags, TriggerFlag::COMMAND));
        REQUIRE(has_flag(flags, TriggerFlag::SPEECH));
        REQUIRE(has_flag(flags, TriggerFlag::GREET));
        REQUIRE_FALSE(has_flag(flags, TriggerFlag::DEATH));
    }
}

TEST_CASE("TriggerSet: Collection operations", "[scripting][triggers]") {
    SECTION("Empty set") {
        TriggerSet set;
        REQUIRE(set.empty());
        REQUIRE(set.size() == 0);
    }

    SECTION("Adding triggers") {
        TriggerSet set;
        auto trigger1 = std::make_shared<TriggerData>();
        trigger1->name = "trigger1";
        trigger1->flags = TriggerFlag::COMMAND;

        auto trigger2 = std::make_shared<TriggerData>();
        trigger2->name = "trigger2";
        trigger2->flags = TriggerFlag::SPEECH;

        set.add(trigger1);
        set.add(trigger2);

        REQUIRE(set.size() == 2);
        REQUIRE_FALSE(set.empty());
    }

    SECTION("find_by_flag works") {
        TriggerSet set;

        auto trigger1 = std::make_shared<TriggerData>();
        trigger1->name = "cmd_trigger";
        trigger1->flags = TriggerFlag::COMMAND;

        auto trigger2 = std::make_shared<TriggerData>();
        trigger2->name = "speech_trigger";
        trigger2->flags = TriggerFlag::SPEECH;

        auto trigger3 = std::make_shared<TriggerData>();
        trigger3->name = "cmd_speech_trigger";
        trigger3->flags = TriggerFlag::COMMAND | TriggerFlag::SPEECH;

        set.add(trigger1);
        set.add(trigger2);
        set.add(trigger3);

        auto cmd_triggers = set.find_by_flag(TriggerFlag::COMMAND);
        REQUIRE(cmd_triggers.size() == 2); // trigger1 and trigger3

        auto speech_triggers = set.find_by_flag(TriggerFlag::SPEECH);
        REQUIRE(speech_triggers.size() == 2); // trigger2 and trigger3

        auto death_triggers = set.find_by_flag(TriggerFlag::DEATH);
        REQUIRE(death_triggers.empty());
    }
}

