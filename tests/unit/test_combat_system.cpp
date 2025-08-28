/***************************************************************************
 *   File: tests/unit/test_combat_system.cpp            Part of FieryMUD *
 *  Usage: Unit tests for combat system calculations                       *
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

TEST_CASE("Combat System - Class Specific Bonuses", "[combat][unit]") {
    SECTION("Warrior bonuses at different levels") {
        auto mods_1 = CombatSystem::get_class_combat_bonus(CharacterClass::Warrior, 1);
        auto mods_10 = CombatSystem::get_class_combat_bonus(CharacterClass::Warrior, 10);
        auto mods_20 = CombatSystem::get_class_combat_bonus(CharacterClass::Warrior, 20);
        
        REQUIRE(mods_1.hit_bonus == Approx(0.75));
        REQUIRE(mods_1.damage_bonus == Approx(0.5));
        
        REQUIRE(mods_10.hit_bonus == Approx(7.5));
        REQUIRE(mods_10.damage_bonus == Approx(5.0));
        REQUIRE(mods_10.armor_class_bonus == Approx(2.5));
        
        REQUIRE(mods_20.hit_bonus == Approx(15.0));
        REQUIRE(mods_20.damage_bonus == Approx(10.0));
        REQUIRE(mods_20.armor_class_bonus == Approx(5.0));
        REQUIRE(mods_20.critical_chance > mods_1.critical_chance);
    }

    SECTION("Sorcerer bonuses (minimal physical combat)") {
        auto mods = CombatSystem::get_class_combat_bonus(CharacterClass::Sorcerer, 10);
        
        REQUIRE(mods.hit_bonus == Approx(2.5));  // Lower than warrior
        REQUIRE(mods.damage_bonus == Approx(1.0));  // Much lower than warrior
        REQUIRE(mods.armor_class_bonus == Approx(0.0));  // No AC improvement
    }

    SECTION("Rogue bonuses (high precision)") {
        auto mods = CombatSystem::get_class_combat_bonus(CharacterClass::Rogue, 10);
        
        REQUIRE(mods.hit_bonus == Approx(6.0));  // High precision
        REQUIRE(mods.damage_bonus == Approx(3.0));  // Moderate damage
        REQUIRE(mods.critical_chance > 0.1);  // Higher crit chance
    }

    SECTION("Cleric bonuses (moderate with some defense)") {
        auto mods = CombatSystem::get_class_combat_bonus(CharacterClass::Cleric, 10);
        
        REQUIRE(mods.hit_bonus == Approx(5.0));  // Moderate attack
        REQUIRE(mods.damage_bonus == Approx(2.5));  // Moderate damage
        REQUIRE(mods.damage_resistance > 0.0);  // Some damage resistance
    }
}

TEST_CASE("Combat System - Race Specific Bonuses", "[combat][unit]") {
    SECTION("Human bonuses (versatile)") {
        auto mods = CombatSystem::get_race_combat_bonus(CharacterRace::Human);
        
        REQUIRE(mods.hit_bonus == Approx(1.0));
        REQUIRE(mods.damage_bonus == Approx(1.0));
        REQUIRE(mods.armor_class_bonus == Approx(0.5));
    }

    SECTION("Elf bonuses (dexterous and magical)") {
        auto mods = CombatSystem::get_race_combat_bonus(CharacterRace::Elf);
        
        REQUIRE(mods.hit_bonus == Approx(2.0));  // Good precision
        REQUIRE(mods.armor_class_bonus == Approx(1.5));  // Natural grace
        REQUIRE(mods.critical_chance == Approx(0.02));  // Bonus crit chance
    }

    SECTION("Dwarf bonuses (tough and strong)") {
        auto mods = CombatSystem::get_race_combat_bonus(CharacterRace::Dwarf);
        
        REQUIRE(mods.damage_bonus == Approx(2.0));  // Strong attacks
        REQUIRE(mods.damage_resistance == Approx(0.05));  // 5% damage resistance
        REQUIRE(mods.armor_class_bonus == Approx(0.0));  // Slow but tough
    }

    SECTION("Halfling bonuses (lucky and evasive)") {
        auto mods = CombatSystem::get_race_combat_bonus(CharacterRace::Halfling);
        
        REQUIRE(mods.damage_bonus == Approx(-0.5));  // Smaller build
        REQUIRE(mods.armor_class_bonus == Approx(2.0));  // Hard to hit
        REQUIRE(mods.critical_chance == Approx(0.03));  // Lucky crits
    }
}

TEST_CASE("Combat System - String to Enum Conversion", "[combat][unit]") {
    SECTION("Character class conversion") {
        REQUIRE(CombatSystem::string_to_class("warrior") == CharacterClass::Warrior);
        REQUIRE(CombatSystem::string_to_class("cleric") == CharacterClass::Cleric);
        REQUIRE(CombatSystem::string_to_class("sorcerer") == CharacterClass::Sorcerer);
        REQUIRE(CombatSystem::string_to_class("rogue") == CharacterClass::Rogue);
        
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

TEST_CASE("Combat System - Combat Modifier Combination", "[combat][unit]") {
    SECTION("Combat modifiers addition") {
        CombatModifiers mod1;
        mod1.hit_bonus = 2.0;
        mod1.damage_bonus = 1.5;
        mod1.armor_class_bonus = 1.0;
        mod1.critical_chance = 0.05;
        mod1.damage_resistance = 0.1;
        
        CombatModifiers mod2;
        mod2.hit_bonus = 3.0;
        mod2.damage_bonus = 2.0;
        mod2.armor_class_bonus = 0.5;
        mod2.critical_chance = 0.03;
        mod2.damage_resistance = 0.05;
        
        CombatModifiers result = mod1 + mod2;
        
        REQUIRE(result.hit_bonus == Approx(5.0));
        REQUIRE(result.damage_bonus == Approx(3.5));
        REQUIRE(result.armor_class_bonus == Approx(1.5));
        REQUIRE(result.critical_chance == Approx(0.08));
        REQUIRE(result.damage_resistance == Approx(0.15));
    }

    SECTION("Critical chance caps at 95%") {
        CombatModifiers mod1;
        mod1.critical_chance = 0.90;
        
        CombatModifiers mod2;
        mod2.critical_chance = 0.10;
        
        CombatModifiers result = mod1 + mod2;
        REQUIRE(result.critical_chance == Approx(0.95));  // Capped
    }

    SECTION("Damage resistance caps at 90%") {
        CombatModifiers mod1;
        mod1.damage_resistance = 0.85;
        
        CombatModifiers mod2;
        mod2.damage_resistance = 0.20;
        
        CombatModifiers result = mod1 + mod2;
        REQUIRE(result.damage_resistance == Approx(0.90));  // Capped
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