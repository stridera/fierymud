#include "../src/database/world_queries.hpp"
#include "../src/core/ability_executor.hpp"
#include "../src/core/formula_parser.hpp"

#include <catch2/catch_test_macros.hpp>

using namespace WorldQueries;
using namespace FieryMUD;

TEST_CASE("CommandData: Structure and Fields", "[command][ability][unit]") {
    SECTION("Default CommandData values") {
        CommandData cmd;
        REQUIRE(cmd.name.empty());
        REQUIRE(cmd.aliases.empty());
        REQUIRE(cmd.category.empty());
        REQUIRE(cmd.description.empty());
        REQUIRE(cmd.usage.empty());
        REQUIRE(cmd.immortal_only == false);
        REQUIRE(cmd.permissions.empty());
        REQUIRE_FALSE(cmd.ability_id.has_value());
    }

    SECTION("CommandData with ability_id") {
        CommandData cmd;
        cmd.name = "hide";
        cmd.category = "SKILLS";
        cmd.description = "Attempt to hide in the shadows";
        cmd.ability_id = 42;

        REQUIRE(cmd.name == "hide");
        REQUIRE(cmd.category == "SKILLS");
        REQUIRE(cmd.ability_id.has_value());
        REQUIRE(cmd.ability_id.value() == 42);
    }

    SECTION("CommandData without ability_id (system command)") {
        CommandData cmd;
        cmd.name = "quit";
        cmd.category = "SYSTEM";
        cmd.description = "Leave the game";
        // No ability_id - this is a pure system command

        REQUIRE(cmd.name == "quit");
        REQUIRE_FALSE(cmd.ability_id.has_value());
    }

    SECTION("CommandData with aliases") {
        CommandData cmd;
        cmd.name = "look";
        cmd.aliases = {"l", "examine", "ex"};

        REQUIRE(cmd.aliases.size() == 3);
        REQUIRE(cmd.aliases[0] == "l");
        REQUIRE(cmd.aliases[1] == "examine");
        REQUIRE(cmd.aliases[2] == "ex");
    }

    SECTION("CommandData with permissions") {
        CommandData cmd;
        cmd.name = "shutdown";
        cmd.category = "ADMIN";
        cmd.immortal_only = true;
        cmd.permissions = {"shutdown", "admin"};

        REQUIRE(cmd.immortal_only == true);
        REQUIRE(cmd.permissions.size() == 2);
        REQUIRE(cmd.permissions[0] == "shutdown");
        REQUIRE(cmd.permissions[1] == "admin");
    }
}

TEST_CASE("FormulaContext: Perception and Concealment Variables", "[formula][ability][unit]") {
    SECTION("Default perception and concealment values") {
        FormulaContext ctx;

        REQUIRE(ctx.perception == 0);
        REQUIRE(ctx.concealment == 0);
        REQUIRE(ctx.target_perception == 0);
        REQUIRE(ctx.target_concealment == 0);
    }

    SECTION("Perception formula evaluation") {
        FormulaContext ctx;
        ctx.perception = 50;
        ctx.skill_level = 75;

        // Test perception variable
        auto result = FormulaParser::evaluate("perception", ctx);
        REQUIRE(result.has_value());
        REQUIRE(result.value() == 50);

        // Test formula with perception
        result = FormulaParser::evaluate("perception + skill / 2", ctx);
        REQUIRE(result.has_value());
        REQUIRE(result.value() == 88); // 50 + 75/2 = 50 + 37.5 = 87.5 -> 88
    }

    SECTION("Concealment formula evaluation") {
        FormulaContext ctx;
        ctx.concealment = 60;
        ctx.dex_bonus = 3;

        // Test concealment variable
        auto result = FormulaParser::evaluate("concealment", ctx);
        REQUIRE(result.has_value());
        REQUIRE(result.value() == 60);

        // Test formula: concealment + dex_bonus * 5
        result = FormulaParser::evaluate("concealment + dex_bonus * 5", ctx);
        REQUIRE(result.has_value());
        REQUIRE(result.value() == 75); // 60 + 3*5 = 75
    }

    SECTION("Contested check: perception vs concealment") {
        FormulaContext ctx;
        ctx.perception = 40;           // Actor's perception
        ctx.target_concealment = 55;   // Target's concealment

        // Formula: perception - target_concealment (positive = spotted)
        auto result = FormulaParser::evaluate("perception - target_concealment", ctx);
        REQUIRE(result.has_value());
        REQUIRE(result.value() == -15); // 40 - 55 = -15 (not spotted)

        // Now with higher perception
        ctx.perception = 70;
        result = FormulaParser::evaluate("perception - target_concealment", ctx);
        REQUIRE(result.has_value());
        REQUIRE(result.value() == 15); // 70 - 55 = 15 (spotted)
    }

    SECTION("Skulk-style detection formula") {
        FormulaContext ctx;
        ctx.skill_level = 80;          // Skulk skill
        ctx.dex_bonus = 4;             // Dexterity modifier
        ctx.target_perception = 45;    // Observer's perception

        // Skulk formula: skill * 2 + dex_bonus * 5 vs target_perception
        // This represents: "perceiver.perception vs sneak skill * 2"
        auto concealment_result = FormulaParser::evaluate("skill * 2 + dex_bonus * 5", ctx);
        REQUIRE(concealment_result.has_value());
        REQUIRE(concealment_result.value() == 180); // 80*2 + 4*5 = 160 + 20 = 180

        // Detection check
        auto detection_result = FormulaParser::evaluate("target_perception - (skill * 2)", ctx);
        REQUIRE(detection_result.has_value());
        REQUIRE(detection_result.value() == -115); // 45 - 160 = -115 (not detected)
    }
}

TEST_CASE("AbilityData: Toggle Skills", "[ability][toggle][unit]") {
    SECTION("Toggle ability structure") {
        AbilityData ability;
        ability.id = 1;
        ability.name = "Hide";
        ability.plain_name = "hide";
        ability.type = AbilityType::Skill;
        ability.is_toggle = true;
        ability.violent = false;

        REQUIRE(ability.id == 1);
        REQUIRE(ability.name == "Hide");
        REQUIRE(ability.plain_name == "hide");
        REQUIRE(ability.type == AbilityType::Skill);
        REQUIRE(ability.is_toggle == true);
        REQUIRE(ability.violent == false);
    }

    SECTION("Non-toggle ability structure") {
        AbilityData ability;
        ability.id = 2;
        ability.name = "Backstab";
        ability.plain_name = "backstab";
        ability.type = AbilityType::Skill;
        ability.is_toggle = false;
        ability.violent = true;

        REQUIRE(ability.type == AbilityType::Skill);
        REQUIRE(ability.is_toggle == false);
        REQUIRE(ability.violent == true);
    }

    SECTION("Spell vs Skill type differentiation") {
        AbilityData spell;
        spell.name = "Fireball";
        spell.type = AbilityType::Spell;

        AbilityData skill;
        skill.name = "Hide";
        skill.type = AbilityType::Skill;

        AbilityData chant;
        chant.name = "Battle Cry";
        chant.type = AbilityType::Chant;

        // Spells should NOT be executable as direct commands
        REQUIRE(spell.type == AbilityType::Spell);

        // Skills, chants, songs CAN be executed as direct commands
        REQUIRE(skill.type != AbilityType::Spell);
        REQUIRE(chant.type != AbilityType::Spell);
    }
}

TEST_CASE("Command-Ability Linkage: Data Flow", "[command][ability][integration][unit]") {
    SECTION("Command with explicit abilityId links to correct ability") {
        // Simulate the linkage flow
        CommandData cmd;
        cmd.name = "hide";
        cmd.category = "SKILLS";
        cmd.ability_id = 42;

        AbilityData ability;
        ability.id = 42;
        ability.name = "Hide";
        ability.plain_name = "hide";
        ability.type = AbilityType::Skill;
        ability.is_toggle = true;

        // Verify the linkage
        REQUIRE(cmd.ability_id.has_value());
        REQUIRE(cmd.ability_id.value() == ability.id);

        // The C++ dispatch would use:
        // 1. Check C++ registered commands (not found)
        // 2. Check Command table for abilityId
        // 3. If abilityId exists, use get_ability(abilityId)
        // 4. Else, fallback to get_ability_by_name(command_name)
    }

    SECTION("Command without abilityId falls back to name lookup") {
        CommandData cmd;
        cmd.name = "skulk";
        cmd.category = "SKILLS";
        // No ability_id set

        AbilityData ability;
        ability.id = 99;
        ability.name = "Skulk";
        ability.plain_name = "skulk";  // Matches command name
        ability.type = AbilityType::Skill;

        REQUIRE_FALSE(cmd.ability_id.has_value());
        // Fallback: match command.name to ability.plain_name
        REQUIRE(cmd.name == ability.plain_name);
    }

    SECTION("Command name different from ability name with explicit link") {
        // This demonstrates the power of explicit abilityId FK
        CommandData cmd;
        cmd.name = "stab";  // Short command name
        cmd.ability_id = 55;

        AbilityData ability;
        ability.id = 55;
        ability.name = "Backstab Attack";  // Full ability name (different!)
        ability.plain_name = "backstab_attack";
        ability.type = AbilityType::Skill;
        ability.violent = true;

        // Without explicit link: cmd.name != ability.plain_name (wouldn't match)
        REQUIRE(cmd.name != ability.plain_name);

        // With explicit link: abilityId connects them directly
        REQUIRE(cmd.ability_id.value() == ability.id);
    }
}

TEST_CASE("Focus Stat: Spell Memorization", "[ability][stats][unit]") {
    SECTION("Focus stat in formula context") {
        FormulaContext ctx;
        ctx.custom_vars["focus"] = 25;
        ctx.skill_level = 50;  // Meditate skill

        // Memorization speed formula: base_time - (focus / 10)
        auto result = FormulaParser::evaluate("100 - focus / 10", ctx);
        // Note: focus needs to be accessed via custom_vars
    }

    SECTION("Meditate improves focus") {
        // Meditate effect formula: skill / 2
        FormulaContext ctx;
        ctx.skill_level = 60;  // Meditate skill at 60%

        auto focus_bonus = FormulaParser::evaluate("skill / 2", ctx);
        REQUIRE(focus_bonus.has_value());
        REQUIRE(focus_bonus.value() == 30);  // 60/2 = 30 focus bonus
    }
}
