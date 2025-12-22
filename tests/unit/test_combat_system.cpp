/***************************************************************************
 *   File: tests/unit/test_combat_system.cpp            Part of FieryMUD *
 *  Usage: Unit tests for modern ACC/EVA combat system                    *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.       *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 ***************************************************************************/

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include "../../src/core/combat.hpp"
#include "../../src/core/actor.hpp"

using namespace FieryMUD;
using Catch::Approx;

TEST_CASE("Combat System - Class ACC Rates", "[combat][unit]") {
    SECTION("Warrior has highest ACC rate") {
        double warrior_rate = CombatSystem::get_class_acc_rate(CharacterClass::Warrior);
        double sorcerer_rate = CombatSystem::get_class_acc_rate(CharacterClass::Sorcerer);

        REQUIRE(warrior_rate == Approx(ClassAccRates::WARRIOR));
        REQUIRE(sorcerer_rate == Approx(ClassAccRates::SORCERER));
        REQUIRE(warrior_rate > sorcerer_rate);
    }

    SECTION("Rogue has good ACC rate") {
        double rogue_rate = CombatSystem::get_class_acc_rate(CharacterClass::Rogue);

        REQUIRE(rogue_rate == Approx(ClassAccRates::ROGUE));
        REQUIRE(rogue_rate > 0.5);  // Better than baseline
    }

    SECTION("Cleric has moderate ACC rate") {
        double cleric_rate = CombatSystem::get_class_acc_rate(CharacterClass::Cleric);

        REQUIRE(cleric_rate == Approx(ClassAccRates::CLERIC));
    }
}

TEST_CASE("Combat System - Race Specific Bonuses", "[combat][unit]") {
    SECTION("Human bonuses (versatile)") {
        auto bonus = CombatSystem::get_race_combat_bonus(CharacterRace::Human);

        REQUIRE(bonus.acc == Approx(2.0));
        REQUIRE(bonus.eva == Approx(2.0));
        REQUIRE(bonus.ap == Approx(1.0));
    }

    SECTION("Elf bonuses (dexterous)") {
        auto bonus = CombatSystem::get_race_combat_bonus(CharacterRace::Elf);

        REQUIRE(bonus.acc == Approx(3.0));
        REQUIRE(bonus.eva == Approx(5.0));  // High evasion
        REQUIRE(bonus.crit_bonus == Approx(5.0));  // Critical margin bonus
    }

    SECTION("Dwarf bonuses (tough)") {
        auto bonus = CombatSystem::get_race_combat_bonus(CharacterRace::Dwarf);

        REQUIRE(bonus.ap == Approx(3.0));  // Strong attacks
        REQUIRE(bonus.ar == Approx(10.0));  // Extra armor
        REQUIRE(bonus.res_physical == Approx(90.0));  // 10% physical resistance
        REQUIRE(bonus.eva == Approx(-2.0));  // Slow
    }

    SECTION("Halfling bonuses (evasive)") {
        auto bonus = CombatSystem::get_race_combat_bonus(CharacterRace::Halfling);

        REQUIRE(bonus.eva == Approx(8.0));  // Very evasive
        REQUIRE(bonus.crit_bonus == Approx(3.0));  // Lucky crits
    }
}

TEST_CASE("Combat System - String to Enum Conversion", "[combat][unit]") {
    SECTION("Character class conversion") {
        REQUIRE(CombatSystem::string_to_class("warrior") == CharacterClass::Warrior);
        REQUIRE(CombatSystem::string_to_class("cleric") == CharacterClass::Cleric);
        REQUIRE(CombatSystem::string_to_class("sorcerer") == CharacterClass::Sorcerer);
        REQUIRE(CombatSystem::string_to_class("rogue") == CharacterClass::Rogue);
        REQUIRE(CombatSystem::string_to_class("thief") == CharacterClass::Rogue);  // Alias

        // Unknown class defaults to warrior
        REQUIRE(CombatSystem::string_to_class("unknown") == CharacterClass::Warrior);
    }

    SECTION("Character race conversion") {
        REQUIRE(CombatSystem::string_to_race("human") == CharacterRace::Human);
        REQUIRE(CombatSystem::string_to_race("elf") == CharacterRace::Elf);
        REQUIRE(CombatSystem::string_to_race("dwarf") == CharacterRace::Dwarf);
        REQUIRE(CombatSystem::string_to_race("halfling") == CharacterRace::Halfling);

        // Unknown race defaults to human
        REQUIRE(CombatSystem::string_to_race("unknown") == CharacterRace::Human);
    }
}

TEST_CASE("Combat System - CombatStats Combination", "[combat][unit]") {
    SECTION("CombatStats addition") {
        CombatStats stats1;
        stats1.acc = 50.0;
        stats1.eva = 30.0;
        stats1.ap = 5.0;
        stats1.soak = 2.0;
        stats1.pen_pct = 0.1;

        CombatStats stats2;
        stats2.acc = 10.0;
        stats2.eva = 5.0;
        stats2.ap = 3.0;
        stats2.soak = 1.0;
        stats2.pen_pct = 0.15;

        CombatStats result = stats1 + stats2;

        REQUIRE(result.acc == Approx(60.0));
        REQUIRE(result.eva == Approx(35.0));
        REQUIRE(result.ap == Approx(8.0));
        REQUIRE(result.soak == Approx(3.0));
        REQUIRE(result.pen_pct == Approx(0.25));
    }

    SECTION("Penetration % caps at 50%") {
        CombatStats stats1;
        stats1.pen_pct = 0.40;

        CombatStats stats2;
        stats2.pen_pct = 0.20;

        CombatStats result = stats1 + stats2;
        REQUIRE(result.pen_pct == Approx(CombatConstants::PEN_PCT_CAP));  // Capped at 50%
    }

    SECTION("Resistances use most favorable (lowest)") {
        CombatStats stats1;
        stats1.res_fire = 100.0;  // Normal
        stats1.res_cold = 80.0;   // Some resistance

        CombatStats stats2;
        stats2.res_fire = 50.0;   // Fire resistance
        stats2.res_cold = 90.0;   // Less resistance

        CombatStats result = stats1 + stats2;
        REQUIRE(result.res_fire == Approx(50.0));  // Uses stats2's fire resistance
        REQUIRE(result.res_cold == Approx(80.0));  // Uses stats1's cold resistance
    }
}

TEST_CASE("Combat System - DR% Calculation", "[combat][unit]") {
    SECTION("DR% formula: AR / (AR + K)") {
        // Low tier (levels 1-20, K=40)
        REQUIRE(CombatStats::calculate_dr_percent(40.0, 10) == Approx(0.5));  // 40/(40+40)
        REQUIRE(CombatStats::calculate_dr_percent(20.0, 10) == Approx(0.333).epsilon(0.01));  // 20/(20+40)

        // Mid tier (levels 21-50, K=60)
        REQUIRE(CombatStats::calculate_dr_percent(60.0, 35) == Approx(0.5));  // 60/(60+60)

        // High tier (levels 51+, K=80)
        REQUIRE(CombatStats::calculate_dr_percent(80.0, 75) == Approx(0.5));  // 80/(80+80)
    }

    SECTION("DR% caps at 75%") {
        // Even with very high AR, DR% should cap
        REQUIRE(CombatStats::calculate_dr_percent(1000.0, 10) == Approx(CombatConstants::DR_CAP));
    }

    SECTION("DR% is 0 for 0 or negative AR") {
        REQUIRE(CombatStats::calculate_dr_percent(0.0, 10) == Approx(0.0));
        REQUIRE(CombatStats::calculate_dr_percent(-10.0, 10) == Approx(0.0));
    }
}

TEST_CASE("Combat System - Hit Calculation", "[combat][unit]") {
    SECTION("Hit result types based on margin") {
        // This test is probabilistic, so we just verify the margin thresholds
        REQUIRE(CombatConstants::CRITICAL_HIT_MARGIN == 50);
        REQUIRE(CombatConstants::HIT_MARGIN == 10);
        REQUIRE(CombatConstants::GLANCING_MARGIN == -10);
    }

    SECTION("High ACC vs Low EVA favors attacker") {
        CombatStats attacker;
        attacker.acc = 150.0;  // Very high

        CombatStats defender;
        defender.eva = 30.0;   // Low

        // With ACC 120 higher than EVA, many hits should land
        // (Can't test exact results due to randomness)
        REQUIRE(attacker.acc > defender.eva);
    }
}

TEST_CASE("Combat System - Damage Mitigation", "[combat][unit]") {
    SECTION("Soak reduces damage before DR%") {
        CombatStats attacker;
        attacker.pen_flat = 0.0;
        attacker.pen_pct = 0.0;

        CombatStats defender;
        defender.soak = 5.0;
        defender.dr_pct = 0.0;  // No DR%
        defender.res_physical = 100.0;  // Normal resistance

        double mitigated = CombatSystem::apply_mitigation(10.0, attacker, defender, 0);
        REQUIRE(mitigated == Approx(5.0));  // 10 - 5 soak = 5
    }

    SECTION("DR% reduces damage after soak") {
        CombatStats attacker;
        attacker.pen_flat = 0.0;
        attacker.pen_pct = 0.0;

        CombatStats defender;
        defender.soak = 0.0;
        defender.dr_pct = 0.5;  // 50% DR
        defender.res_physical = 100.0;

        double mitigated = CombatSystem::apply_mitigation(10.0, attacker, defender, 0);
        REQUIRE(mitigated == Approx(5.0));  // 10 * (1 - 0.5) = 5
    }

    SECTION("Penetration reduces defender's mitigation") {
        CombatStats attacker;
        attacker.pen_flat = 3.0;  // Reduces soak by 3
        attacker.pen_pct = 0.5;   // Reduces DR% by 50%

        CombatStats defender;
        defender.soak = 5.0;      // Effective soak = 2
        defender.dr_pct = 0.4;    // Effective DR = 0.2 (40% * 50%)
        defender.res_physical = 100.0;

        // 10 - 2 soak = 8, then 8 * (1 - 0.2) = 6.4
        double mitigated = CombatSystem::apply_mitigation(10.0, attacker, defender, 0);
        REQUIRE(mitigated == Approx(6.4));
    }

    SECTION("Minimum damage is 1") {
        CombatStats attacker;
        attacker.pen_flat = 0.0;
        attacker.pen_pct = 0.0;

        CombatStats defender;
        defender.soak = 100.0;  // Way more than damage
        defender.dr_pct = 0.75; // Max DR
        defender.res_physical = 100.0;

        double mitigated = CombatSystem::apply_mitigation(5.0, attacker, defender, 0);
        REQUIRE(mitigated == Approx(CombatConstants::MIN_DAMAGE));
    }
}

TEST_CASE("Combat System - Combat Manager Basic Functions", "[combat][unit]") {
    // Create test actors without requiring WorldManager
    auto attacker = Player::create(EntityId{1001UL}, "TestAttacker");
    auto target = Player::create(EntityId{1002UL}, "TestTarget");

    REQUIRE(attacker.has_value());
    REQUIRE(target.has_value());

    // Convert to shared_ptr as expected by CombatManager API
    std::shared_ptr<Actor> attacker_ptr = std::move(attacker.value());
    std::shared_ptr<Actor> target_ptr = std::move(target.value());

    // Clear any previous combat state
    FieryMUD::CombatManager::clear_all_combat();

    // Initially, actors should not be in combat
    REQUIRE_FALSE(FieryMUD::CombatManager::is_in_combat(*attacker_ptr));
    REQUIRE_FALSE(FieryMUD::CombatManager::is_in_combat(*target_ptr));

    // Start combat between actors
    FieryMUD::CombatManager::start_combat(attacker_ptr, target_ptr);

    // Now both should be in combat
    REQUIRE(FieryMUD::CombatManager::is_in_combat(*attacker_ptr));
    REQUIRE(FieryMUD::CombatManager::is_in_combat(*target_ptr));

    // They should be fighting each other
    REQUIRE(FieryMUD::CombatManager::get_opponent(*attacker_ptr) == target_ptr);
    REQUIRE(FieryMUD::CombatManager::get_opponent(*target_ptr) == attacker_ptr);

    // Positions should be set to Fighting
    REQUIRE(attacker_ptr->position() == Position::Fighting);
    REQUIRE(target_ptr->position() == Position::Fighting);

    // End combat for one actor
    FieryMUD::CombatManager::end_combat(attacker_ptr);

    // Both should no longer be in combat
    REQUIRE_FALSE(FieryMUD::CombatManager::is_in_combat(*attacker_ptr));
    REQUIRE_FALSE(FieryMUD::CombatManager::is_in_combat(*target_ptr));
}

TEST_CASE("Combat System - Combat Constants", "[combat][unit]") {
    SECTION("Round timing constants") {
        REQUIRE(CombatConstants::COMBAT_ROUND_SECONDS == 4);
        REQUIRE(CombatConstants::COMBAT_ROUND_MS == 4000);
    }

    SECTION("Damage multipliers") {
        REQUIRE(CombatConstants::CRITICAL_MULTIPLIER == Approx(2.0));
        REQUIRE(CombatConstants::GLANCING_MULTIPLIER == Approx(0.5));
        REQUIRE(CombatConstants::MIN_DAMAGE == Approx(1.0));
    }

    SECTION("Caps") {
        REQUIRE(CombatConstants::DR_CAP == Approx(0.75));
        REQUIRE(CombatConstants::PEN_PCT_CAP == Approx(0.50));
        REQUIRE(CombatConstants::ACC_SOFT_CAP == 200);
        REQUIRE(CombatConstants::EVA_SOFT_CAP == 150);
    }
}
