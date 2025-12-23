/***************************************************************************
 *   File: tests/integration/test_combat_system.cpp        Part of FieryMUD *
 *  Usage: Integration tests for combat system with test harness           *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.       *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 ***************************************************************************/

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include "../../src/core/combat.hpp"
#include "../../src/core/actor.hpp"
#include "../../src/world/room.hpp"
#include "../common/mock_game_session.hpp"

using namespace FieryMUD;
using Catch::Approx;

TEST_CASE("Combat System - Experience Calculation", "[combat][integration]") {
    UnifiedTestHarness::run_unit_test([&]() {
        auto session = UnifiedTestHarness::create_session();
    
        // Create attacker (level 5)
        auto attacker = Player::create(EntityId{100}, "Attacker");
        REQUIRE(attacker.has_value());
        attacker.value()->stats().level = 5;
        
        // Create target (level 7)
        auto target = Player::create(EntityId{101}, "Target");
        REQUIRE(target.has_value());
        target.value()->stats().level = 7;
        
        // Experience gain for higher level target
        long exp_gain = CombatSystem::calculate_experience_gain(*attacker.value(), *target.value());
        // Base: 7 * 100 = 700, multiplier for +2 levels = 1.2, total = 840
        REQUIRE(exp_gain == 840);
        
        // Experience gain for same level target
        target.value()->stats().level = 5;
        exp_gain = CombatSystem::calculate_experience_gain(*attacker.value(), *target.value());
        // Base: 5 * 100 = 500, no multiplier = 500
        REQUIRE(exp_gain == 500);
        
        // Reduced experience for much lower level target
        target.value()->stats().level = 1;  // 4 levels lower
        exp_gain = CombatSystem::calculate_experience_gain(*attacker.value(), *target.value());
        // Base: 1 * 100 = 100, multiplier for -4 levels = 0.8, total = 80
        REQUIRE(exp_gain == 80);
    });
}

TEST_CASE("Combat System - Warrior Extra Attack", "[combat][integration]") {
    UnifiedTestHarness::run_unit_test([&]() {
        auto session = UnifiedTestHarness::create_session();
        
        auto warrior = Player::create(EntityId{100}, "Warrior");
        REQUIRE(warrior.has_value());
        warrior.value()->set_class("warrior");
        
        warrior.value()->stats().level = 5;
        REQUIRE_FALSE(ClassAbilities::warrior_extra_attack_available(*warrior.value()));
        
        warrior.value()->stats().level = 6;
        REQUIRE(ClassAbilities::warrior_extra_attack_available(*warrior.value()));
        
        warrior.value()->stats().level = 11;
        REQUIRE(ClassAbilities::warrior_extra_attack_available(*warrior.value()));
    });
}

TEST_CASE("Combat System - Rogue Sneak Attack", "[combat][integration]") {
    UnifiedTestHarness::run_unit_test([&]() {
        auto session = UnifiedTestHarness::create_session();
        
        auto rogue = Player::create(EntityId{100}, "Rogue");
        REQUIRE(rogue.has_value());
        rogue.value()->set_class("rogue");
        
        auto target = Player::create(EntityId{101}, "Target");
        REQUIRE(target.has_value());
        
        REQUIRE(ClassAbilities::rogue_sneak_attack_available(*rogue.value(), *target.value()));
        
        // Test damage scaling with level
        double damage_1 = ClassAbilities::rogue_sneak_attack_damage(1);
        double damage_5 = ClassAbilities::rogue_sneak_attack_damage(5);
        double damage_10 = ClassAbilities::rogue_sneak_attack_damage(10);
        
        // Level 1-2 = 1d6, Level 3-4 = 2d6, Level 5-6 = 3d6, etc.
        REQUIRE(damage_1 >= 1.0);
        REQUIRE(damage_1 <= 6.0);  // 1d6 range
        
        REQUIRE(damage_5 >= 3.0);  // At least 3d6 minimum
        REQUIRE(damage_10 >= 5.0); // At least 5d6 minimum
    });
}

TEST_CASE("Combat System - Cleric Turn Undead", "[combat][integration]") {
    UnifiedTestHarness::run_unit_test([&]() {
        auto session = UnifiedTestHarness::create_session();
        
        auto cleric = Player::create(EntityId{100}, "Cleric");
        REQUIRE(cleric.has_value());
        cleric.value()->set_class("cleric");
        
        cleric.value()->stats().level = 1;
        REQUIRE_FALSE(ClassAbilities::cleric_can_turn_undead(*cleric.value()));
        
        cleric.value()->stats().level = 2;
        REQUIRE(ClassAbilities::cleric_can_turn_undead(*cleric.value()));
    });
}

TEST_CASE("Combat System - Warrior vs Sorcerer Combat", "[combat][integration]") {
    UnifiedTestHarness::run_integration_test([&]() {
        auto session = UnifiedTestHarness::create_session();
        
        // Create warrior
        auto warrior = Player::create(EntityId{100}, "Warrior");
        REQUIRE(warrior.has_value());
        warrior.value()->set_class("warrior");
        warrior.value()->set_race("human");
        warrior.value()->stats().level = 10;
        warrior.value()->stats().strength = 16;  // Good strength
        
        // Create sorcerer target
        auto sorcerer = Player::create(EntityId{101}, "Sorcerer");
        REQUIRE(sorcerer.has_value());
        sorcerer.value()->set_class("sorcerer");
        sorcerer.value()->set_race("elf");
        sorcerer.value()->stats().level = 10;
        sorcerer.value()->stats().hit_points = 50;
        
        // Calculate combat stats using modern ACC/EVA system
        CombatStats warrior_stats = CombatSystem::calculate_combat_stats(*warrior.value());
        CombatStats sorcerer_stats = CombatSystem::calculate_combat_stats(*sorcerer.value());

        // Warrior should have significantly higher combat bonuses
        REQUIRE(warrior_stats.acc > sorcerer_stats.acc);     // ACC replaces hit_bonus
        REQUIRE(warrior_stats.ap > sorcerer_stats.ap);        // AP replaces damage_bonus

        // Human warrior should have better armor than elf sorcerer
        REQUIRE(warrior_stats.ar >= sorcerer_stats.ar);       // AR replaces armor_class_bonus
    });
}

TEST_CASE("Combat System - Combat Result Structure", "[combat][integration]") {
    UnifiedTestHarness::run_integration_test([&]() {
        auto session = UnifiedTestHarness::create_session();
        
        auto attacker = Player::create(EntityId{100}, "Attacker");
        REQUIRE(attacker.has_value());
        attacker.value()->set_class("warrior");
        attacker.value()->stats().level = 5;
        attacker.value()->stats().hit_points = 50;
        
        auto target = Player::create(EntityId{101}, "Target");
        REQUIRE(target.has_value());
        target.value()->set_class("sorcerer");
        target.value()->stats().level = 5;
        target.value()->stats().hit_points = 30;
        
        // Perform combat attack (convert unique_ptr to shared_ptr)
        std::shared_ptr<Actor> attacker_ptr = std::move(attacker.value());
        std::shared_ptr<Actor> target_ptr = std::move(target.value());
        CombatResult result = CombatSystem::perform_attack(attacker_ptr, target_ptr);
        
        // Result should have appropriate message content
        REQUIRE_FALSE(result.attacker_message.empty());
        REQUIRE_FALSE(result.target_message.empty());
        REQUIRE_FALSE(result.room_message.empty());
        
        // Result type should be reasonable
        REQUIRE((result.type == CombatResult::Type::Hit || 
                result.type == CombatResult::Type::CriticalHit || 
                result.type == CombatResult::Type::Miss ||
                result.type == CombatResult::Type::Death));
    });
}