/***************************************************************************
 *   File: tests/unit/test_scripting.cpp                      Part of FieryMUD *
 *  Usage: Unit tests for Lua scripting system                              *
 ***************************************************************************/

#include "../src/scripting/script_engine.hpp"
#include "../src/scripting/coroutine_scheduler.hpp"
#include "../src/scripting/triggers/trigger_types.hpp"
#include "../src/scripting/triggers/trigger_data.hpp"
#include "../src/core/ids.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
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

// ============================================================================
// Lua Binding Integration Tests
// ============================================================================

TEST_CASE("Lua Bindings: Enum availability", "[scripting][bindings]") {
    auto& engine = ScriptEngine::instance();
    if (!engine.is_initialized()) {
        engine.initialize();
    }

    SECTION("ObjectType enum is accessible") {
        auto result = engine.execute(
            "return ObjectType.Weapon ~= nil and ObjectType.Armor ~= nil",
            "test_object_type_enum"
        );
        REQUIRE(result.has_value());
        REQUIRE(result.value().get<bool>() == true);
    }

    SECTION("ObjectType has expected values") {
        auto result = engine.execute(R"(
            -- Check that different ObjectTypes have different values
            return ObjectType.Light ~= ObjectType.Weapon and
                   ObjectType.Scroll ~= ObjectType.Staff and
                   ObjectType.Container ~= ObjectType.Food
        )", "test_object_type_values");
        REQUIRE(result.has_value());
        REQUIRE(result.value().get<bool>() == true);
    }

    SECTION("EquipSlot enum is accessible") {
        auto result = engine.execute(
            "return EquipSlot.Head ~= nil and EquipSlot.Wield ~= nil",
            "test_equip_slot_enum"
        );
        REQUIRE(result.has_value());
        REQUIRE(result.value().get<bool>() == true);
    }

    SECTION("ObjectFlag enum is accessible") {
        auto result = engine.execute(
            "return ObjectFlag.Magic ~= nil and ObjectFlag.Cursed ~= nil",
            "test_object_flag_enum"
        );
        REQUIRE(result.has_value());
        REQUIRE(result.value().get<bool>() == true);
    }

    SECTION("Position enum is accessible") {
        auto result = engine.execute(
            "return Position.Standing ~= nil and Position.Sleeping ~= nil",
            "test_position_enum"
        );
        REQUIRE(result.has_value());
        REQUIRE(result.value().get<bool>() == true);
    }

    SECTION("Direction enum is accessible") {
        auto result = engine.execute(
            "return Direction.North ~= nil and Direction.South ~= nil and Direction.Up ~= nil",
            "test_direction_enum"
        );
        REQUIRE(result.has_value());
        REQUIRE(result.value().get<bool>() == true);
    }

    SECTION("SectorType enum is accessible") {
        auto result = engine.execute(
            "return SectorType.City ~= nil and SectorType.Forest ~= nil",
            "test_sector_type_enum"
        );
        REQUIRE(result.has_value());
        REQUIRE(result.value().get<bool>() == true);
    }

    // RoomFlag enum removed - room state is now controlled by individual properties
    // (is_peaceful, allows_magic, base_light_level) rather than a flags enum

    SECTION("MobBehavior and MobTrait enums are accessible") {
        auto result = engine.execute(
            "return MobBehavior.Sentinel ~= nil and MobTrait.Mount ~= nil",
            "test_mob_enum"
        );
        REQUIRE(result.has_value());
        REQUIRE(result.value().get<bool>() == true);
    }

    SECTION("ActorFlag enum is accessible") {
        auto result = engine.execute(
            "return ActorFlag.Invisible ~= nil and ActorFlag.Sanctuary ~= nil",
            "test_actor_flag_enum"
        );
        REQUIRE(result.has_value());
        REQUIRE(result.value().get<bool>() == true);
    }
}

TEST_CASE("Lua Bindings: Usertype registration", "[scripting][bindings]") {
    auto& engine = ScriptEngine::instance();
    if (!engine.is_initialized()) {
        engine.initialize();
    }

    SECTION("Object usertype is registered") {
        auto result = engine.execute(
            "return Object ~= nil",
            "test_object_usertype"
        );
        REQUIRE(result.has_value());
        REQUIRE(result.value().get<bool>() == true);
    }

    SECTION("Container usertype is registered") {
        auto result = engine.execute(
            "return Container ~= nil",
            "test_container_usertype"
        );
        REQUIRE(result.has_value());
        REQUIRE(result.value().get<bool>() == true);
    }

    SECTION("Actor usertype is registered") {
        auto result = engine.execute(
            "return Actor ~= nil",
            "test_actor_usertype"
        );
        REQUIRE(result.has_value());
        REQUIRE(result.value().get<bool>() == true);
    }

    SECTION("Mobile usertype is registered") {
        auto result = engine.execute(
            "return Mobile ~= nil",
            "test_mobile_usertype"
        );
        REQUIRE(result.has_value());
        REQUIRE(result.value().get<bool>() == true);
    }

    SECTION("Player usertype is registered") {
        auto result = engine.execute(
            "return Player ~= nil",
            "test_player_usertype"
        );
        REQUIRE(result.has_value());
        REQUIRE(result.value().get<bool>() == true);
    }

    SECTION("Room usertype is registered") {
        auto result = engine.execute(
            "return Room ~= nil",
            "test_room_usertype"
        );
        REQUIRE(result.has_value());
        REQUIRE(result.value().get<bool>() == true);
    }
}

TEST_CASE("Lua Bindings: Direction helper functions", "[scripting][bindings]") {
    auto& engine = ScriptEngine::instance();
    if (!engine.is_initialized()) {
        engine.initialize();
    }

    SECTION("reverse_direction function exists") {
        auto result = engine.execute(
            "return reverse_direction ~= nil",
            "test_reverse_dir_exists"
        );
        REQUIRE(result.has_value());
        REQUIRE(result.value().get<bool>() == true);
    }

    SECTION("reverse_direction returns opposite") {
        auto result = engine.execute(
            "return reverse_direction(Direction.North) == Direction.South",
            "test_reverse_north"
        );
        REQUIRE(result.has_value());
        REQUIRE(result.value().get<bool>() == true);
    }

    SECTION("reverse_direction works for all cardinal directions") {
        auto result = engine.execute(R"(
            return reverse_direction(Direction.North) == Direction.South and
                   reverse_direction(Direction.South) == Direction.North and
                   reverse_direction(Direction.East) == Direction.West and
                   reverse_direction(Direction.West) == Direction.East and
                   reverse_direction(Direction.Up) == Direction.Down and
                   reverse_direction(Direction.Down) == Direction.Up
        )", "test_reverse_all");
        REQUIRE(result.has_value());
        REQUIRE(result.value().get<bool>() == true);
    }

    SECTION("direction_name function exists") {
        auto result = engine.execute(
            "return direction_name ~= nil",
            "test_dir_name_exists"
        );
        REQUIRE(result.has_value());
        REQUIRE(result.value().get<bool>() == true);
    }

    SECTION("direction_name returns string") {
        auto result = engine.execute(
            "return type(direction_name(Direction.North)) == 'string'",
            "test_dir_name_type"
        );
        REQUIRE(result.has_value());
        REQUIRE(result.value().get<bool>() == true);
    }
}

TEST_CASE("Lua Bindings: Enum comparisons", "[scripting][bindings]") {
    auto& engine = ScriptEngine::instance();
    if (!engine.is_initialized()) {
        engine.initialize();
    }

    SECTION("Position enums can be compared") {
        auto result = engine.execute(R"(
            local standing = Position.Standing
            local fighting = Position.Fighting
            return standing ~= fighting and standing == Position.Standing
        )", "test_position_compare");
        REQUIRE(result.has_value());
        REQUIRE(result.value().get<bool>() == true);
    }

    SECTION("ObjectType enums can be compared") {
        auto result = engine.execute(R"(
            local weapon = ObjectType.Weapon
            local armor = ObjectType.Armor
            return weapon ~= armor and weapon == ObjectType.Weapon
        )", "test_objecttype_compare");
        REQUIRE(result.has_value());
        REQUIRE(result.value().get<bool>() == true);
    }

    SECTION("Direction enums can be compared") {
        auto result = engine.execute(R"(
            local north = Direction.North
            local south = Direction.South
            return north ~= south and north == Direction.North
        )", "test_direction_compare");
        REQUIRE(result.has_value());
        REQUIRE(result.value().get<bool>() == true);
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

// ============================================================================
// TriggerManager Tests
// ============================================================================

#include "../src/scripting/trigger_manager.hpp"

TEST_CASE("TriggerManager: Initialization", "[scripting][triggers][manager]") {
    auto& engine = ScriptEngine::instance();
    if (!engine.is_initialized()) {
        engine.initialize();
    }

    auto& manager = TriggerManager::instance();

    SECTION("Manager can be initialized") {
        bool result = manager.initialize();
        REQUIRE(result == true);
        REQUIRE(manager.is_initialized());
    }

    SECTION("Double initialization is safe") {
        REQUIRE(manager.initialize() == true);
        bool result = manager.initialize();
        REQUIRE(result == true);
    }

    SECTION("Initially has no triggers") {
        REQUIRE(manager.initialize() == true);
        manager.clear_all_triggers();
        REQUIRE(manager.trigger_count() == 0);
    }
}

TEST_CASE("TriggerManager: Trigger registration", "[scripting][triggers][manager]") {
    auto& engine = ScriptEngine::instance();
    if (!engine.is_initialized()) {
        engine.initialize();
    }

    auto& manager = TriggerManager::instance();
    REQUIRE(manager.initialize() == true);
    manager.clear_all_triggers();

    SECTION("Can register a mob trigger") {
        auto trigger = std::make_shared<TriggerData>();
        trigger->name = "test_mob_trigger";
        trigger->attach_type = ScriptType::MOB;
        trigger->flags = TriggerFlag::COMMAND;
        trigger->mob_id = EntityId(30, 42);
        trigger->commands = "return true";

        manager.register_trigger(trigger);
        REQUIRE(manager.trigger_count() == 1);
        REQUIRE(manager.trigger_count(ScriptType::MOB) == 1);
    }

    SECTION("Can retrieve registered triggers") {
        auto trigger = std::make_shared<TriggerData>();
        trigger->name = "speech_trigger";
        trigger->attach_type = ScriptType::MOB;
        trigger->flags = TriggerFlag::SPEECH;
        trigger->mob_id = EntityId(30, 50);
        trigger->commands = "self:say('Hello!')";

        manager.register_trigger(trigger);

        auto triggers = manager.get_mob_triggers(EntityId(30, 50));
        REQUIRE(triggers.size() == 1);
        REQUIRE(triggers.triggers[0]->name == "speech_trigger");
    }

    SECTION("Can find triggers by flag") {
        auto cmd_trigger = std::make_shared<TriggerData>();
        cmd_trigger->name = "cmd";
        cmd_trigger->attach_type = ScriptType::MOB;
        cmd_trigger->flags = TriggerFlag::COMMAND;
        cmd_trigger->mob_id = EntityId(30, 60);
        cmd_trigger->commands = "return true";

        auto greet_trigger = std::make_shared<TriggerData>();
        greet_trigger->name = "greet";
        greet_trigger->attach_type = ScriptType::MOB;
        greet_trigger->flags = TriggerFlag::GREET;
        greet_trigger->mob_id = EntityId(30, 60);
        greet_trigger->commands = "return true";

        manager.register_trigger(cmd_trigger);
        manager.register_trigger(greet_trigger);

        auto found = manager.find_triggers(EntityId(30, 60), ScriptType::MOB, TriggerFlag::COMMAND);
        REQUIRE(found.size() == 1);
        REQUIRE(found[0]->name == "cmd");

        found = manager.find_triggers(EntityId(30, 60), ScriptType::MOB, TriggerFlag::GREET);
        REQUIRE(found.size() == 1);
        REQUIRE(found[0]->name == "greet");
    }
}

TEST_CASE("TriggerManager: Zone management", "[scripting][triggers][manager]") {
    auto& engine = ScriptEngine::instance();
    if (!engine.is_initialized()) {
        engine.initialize();
    }

    auto& manager = TriggerManager::instance();
    REQUIRE(manager.initialize() == true);
    manager.clear_all_triggers();

    SECTION("Can clear zone triggers") {
        auto trigger1 = std::make_shared<TriggerData>();
        trigger1->name = "zone30_trigger";
        trigger1->attach_type = ScriptType::MOB;
        trigger1->flags = TriggerFlag::COMMAND;
        trigger1->mob_id = EntityId(30, 10);
        trigger1->commands = "return true";

        auto trigger2 = std::make_shared<TriggerData>();
        trigger2->name = "zone31_trigger";
        trigger2->attach_type = ScriptType::MOB;
        trigger2->flags = TriggerFlag::COMMAND;
        trigger2->mob_id = EntityId(31, 10);
        trigger2->commands = "return true";

        manager.register_trigger(trigger1);
        manager.register_trigger(trigger2);
        REQUIRE(manager.trigger_count() == 2);

        manager.clear_zone_triggers(30);
        REQUIRE(manager.trigger_count() == 1);

        auto remaining = manager.get_mob_triggers(EntityId(31, 10));
        REQUIRE(remaining.size() == 1);
    }
}

TEST_CASE("TriggerManager: Script execution context", "[scripting][triggers][manager]") {
    auto& engine = ScriptEngine::instance();
    if (!engine.is_initialized()) {
        engine.initialize();
    }

    SECTION("Script context variables are accessible") {
        // Test that Lua can access cmd and arg variables
        auto result = engine.execute(R"(
            -- Simulate context setup
            cmd = "test"
            arg = "argument"
            return cmd == "test" and arg == "argument"
        )", "test_context_vars");
        REQUIRE(result.has_value());
        REQUIRE(result.value().get<bool>() == true);
    }

    SECTION("Script can return false to halt") {
        auto result = engine.execute("return false", "test_halt");
        REQUIRE(result.has_value());
        REQUIRE(result.value().get<bool>() == false);
    }

    SECTION("Script can return true to continue") {
        auto result = engine.execute("return true", "test_continue");
        REQUIRE(result.has_value());
        REQUIRE(result.value().get<bool>() == true);
    }
}

// ============================================================================
// CoroutineScheduler Tests
// ============================================================================

TEST_CASE("CoroutineScheduler: Basic initialization", "[scripting][coroutines]") {
    auto& scheduler = get_coroutine_scheduler();

    SECTION("Reports zero pending count when uninitialized") {
        REQUIRE(scheduler.pending_count() == 0);
    }

    SECTION("Cancel returns 0 for unknown entities") {
        auto cancelled = scheduler.cancel_for_entity(EntityId{99, 99});
        REQUIRE(cancelled == 0);
    }

    SECTION("Cancel specific coroutine returns false for unknown ID") {
        REQUIRE_FALSE(scheduler.cancel_coroutine(999999));
    }
}

TEST_CASE("CoroutineScheduler: wait() function registration", "[scripting][coroutines]") {
    auto& engine = ScriptEngine::instance();
    if (!engine.is_initialized()) {
        engine.initialize();
    }

    SECTION("wait function is registered") {
        auto& lua = engine.lua_state();
        sol::object wait_func = lua["wait"];
        REQUIRE(wait_func.valid());
        REQUIRE(wait_func.get_type() == sol::type::function);
    }
}

TEST_CASE("CoroutineScheduler: Yielding scripts", "[scripting][coroutines]") {
    auto& engine = ScriptEngine::instance();
    if (!engine.is_initialized()) {
        engine.initialize();
    }

    SECTION("Script with wait() yields the coroutine") {
        // Create a thread for this script
        sol::thread thread = engine.create_thread();
        sol::state_view thread_lua(thread.state());

        // Load a script that calls wait()
        sol::load_result loaded = thread_lua.load(R"(
            local delay = wait(1.5)
            return "completed"
        )", "yield_test");

        REQUIRE(loaded.valid());

        // Create and run coroutine
        sol::coroutine coro(thread.state(), loaded.get<sol::function>());
        auto result = coro();

        // The coroutine should have yielded (because wait() uses sol::yielding)
        REQUIRE(coro.status() == sol::call_status::yielded);

        // The yielded value should be the clamped delay (1.5 seconds)
        REQUIRE(result.valid());
        if (result.return_count() > 0) {
            double delay = result.get<double>(0);
            REQUIRE(delay == Catch::Approx(1.5).margin(0.01));
        }
    }

    SECTION("Script without wait() completes normally") {
        sol::thread thread = engine.create_thread();
        sol::state_view thread_lua(thread.state());

        sol::load_result loaded = thread_lua.load(R"(
            return 42
        )", "no_yield_test");

        REQUIRE(loaded.valid());

        sol::coroutine coro(thread.state(), loaded.get<sol::function>());
        auto result = coro();

        // Should complete normally (not yielded)
        auto status = coro.status();
        REQUIRE(status != sol::call_status::yielded);
        REQUIRE(result.valid());
        // The coroutine completed, but checking return value is more complex
        // Just verify it ran successfully
        REQUIRE(status == sol::call_status::ok);
    }

    SECTION("wait() clamps delay to valid range") {
        sol::thread thread = engine.create_thread();
        sol::state_view thread_lua(thread.state());

        // Test minimum clamping (0.01 seconds should be clamped to MIN_DELAY)
        sol::load_result loaded = thread_lua.load(R"(
            return wait(0.01)
        )", "min_clamp_test");

        REQUIRE(loaded.valid());

        sol::coroutine coro(thread.state(), loaded.get<sol::function>());
        auto result = coro();

        REQUIRE(coro.status() == sol::call_status::yielded);
        if (result.valid() && result.return_count() > 0) {
            double delay = result.get<double>(0);
            // Should be clamped to MIN_DELAY_SECONDS (0.1)
            REQUIRE(delay >= CoroutineScheduler::MIN_DELAY_SECONDS);
        }
    }
}

