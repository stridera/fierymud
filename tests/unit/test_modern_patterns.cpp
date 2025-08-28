/***************************************************************************
 *   File: test_modern_patterns.cpp                       Part of FieryMUD *
 *  Usage: Modern C++23 testing patterns and best practices               *
 ***************************************************************************/

#include "../../src/core/result.hpp"
#include "../../src/core/actor.hpp"
#include "../../src/core/combat.hpp"
#include "../../src/game/login_system.hpp"
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/generators/catch_generators_range.hpp>
#include <ranges>
#include <concepts>

// Modern C++23 concepts for type-safe testing
template<typename T>
concept Testable = requires(T t) {
    t.validate();
    typename T::value_type;
};

template<typename T>
concept ActorLike = std::derived_from<T, Actor>;

/**
 * Class-specific combat bonus testing
 */
TEST_CASE("Combat: Warrior Class Abilities", "[combat][warrior]") {
    auto bonuses = FieryMUD::CombatSystem::get_class_combat_bonus(CharacterClass::Warrior, 10);
    
    SECTION("Basic combat bonus validation") {
        REQUIRE(bonuses.hit_bonus >= 0.0);
        REQUIRE(bonuses.damage_bonus >= 0.0);
        REQUIRE(bonuses.critical_chance >= 0.0);
        REQUIRE(bonuses.critical_chance <= 1.0);
    }
    
    SECTION("Warrior-specific bonuses") {
        REQUIRE(bonuses.hit_bonus >= 5.0); // Warriors should have good hit bonus
        REQUIRE(bonuses.damage_bonus >= 3.0); // And good damage
    }
}

TEST_CASE("Combat: Sorcerer Class Abilities", "[combat][sorcerer]") {
    auto bonuses = FieryMUD::CombatSystem::get_class_combat_bonus(CharacterClass::Sorcerer, 10);
    
    SECTION("Basic combat bonus validation") {
        REQUIRE(bonuses.hit_bonus >= 0.0);
        REQUIRE(bonuses.damage_bonus >= 0.0);
        REQUIRE(bonuses.critical_chance >= 0.0);
        REQUIRE(bonuses.critical_chance <= 1.0);
    }
    
    SECTION("Sorcerer-specific bonuses") {
        REQUIRE(bonuses.hit_bonus <= 3.0); // Sorcerers have lower physical combat
        REQUIRE(bonuses.damage_bonus <= 2.0);
    }
}

TEST_CASE("Combat: Rogue Class Abilities", "[combat][rogue]") {
    auto bonuses = FieryMUD::CombatSystem::get_class_combat_bonus(CharacterClass::Rogue, 10);
    
    SECTION("Basic combat bonus validation") {
        REQUIRE(bonuses.hit_bonus >= 0.0);
        REQUIRE(bonuses.damage_bonus >= 0.0);
        REQUIRE(bonuses.critical_chance >= 0.0);
        REQUIRE(bonuses.critical_chance <= 1.0);
    }
    
    SECTION("Rogue-specific bonuses") {
        REQUIRE(bonuses.critical_chance >= 0.05); // Rogues have higher crit chance
    }
}

TEST_CASE("Combat: Cleric Class Abilities", "[combat][cleric]") {
    auto bonuses = FieryMUD::CombatSystem::get_class_combat_bonus(CharacterClass::Cleric, 10);
    
    SECTION("Basic combat bonus validation") {
        REQUIRE(bonuses.hit_bonus >= 0.0);
        REQUIRE(bonuses.damage_bonus >= 0.0);
        REQUIRE(bonuses.critical_chance >= 0.0);
        REQUIRE(bonuses.critical_chance <= 1.0);
    }
    
    SECTION("Cleric-specific bonuses") {
        REQUIRE(bonuses.damage_resistance >= 0.0); // Clerics have some resistance
    }
}

/**
 * Generator-based property testing
 */
TEST_CASE("Stats: Attribute Modifier Calculation", "[unit][stats][generator]") {
    // Test the D&D-style attribute modifier calculation across all possible values
    
    auto attribute_value = GENERATE(range(1, 26)); // All possible attribute values
    
    int modifier = Stats::attribute_modifier(attribute_value);
    
    SECTION("Modifier calculation properties") {
        // Property: Modifier should be within reasonable bounds
        REQUIRE(modifier >= -5);
        REQUIRE(modifier <= +5);
        
        // Property: Higher attributes should have higher (or equal) modifiers
        if (attribute_value > 10) {
            int lower_modifier = Stats::attribute_modifier(attribute_value - 1);
            REQUIRE(modifier >= lower_modifier);
        }
        
        // Specific cases for D&D standard
        if (attribute_value == 10 || attribute_value == 11) {
            REQUIRE(modifier == 0); // Average attributes have 0 modifier
        }
        if (attribute_value == 18) {
            REQUIRE(modifier == 4); // Exceptional human attribute
        }
        if (attribute_value <= 3) {
            REQUIRE(modifier <= -4); // Very low attributes have severe penalties
        }
    }
    
    INFO("Attribute " << attribute_value << " has modifier " << modifier);
}

/**
 * Ranges-based testing with modern C++
 */
TEST_CASE("Actor: Inventory Management with Ranges", "[unit][actor][ranges]") {
    auto player = Player::create(EntityId{1001}, "TestPlayer");
    REQUIRE(player.has_value());
    auto& actor = *player.value();
    
    SECTION("Inventory operations with ranges") {
        // Create test objects
        std::vector<std::shared_ptr<Object>> test_objects;
        for (int i = 1; i <= 10; ++i) {
            auto obj_result = Object::create(EntityId{static_cast<uint64_t>(1000 + i)}, 
                                           fmt::format("TestItem{}", i), ObjectType::Other);
            REQUIRE(obj_result.has_value());
            test_objects.push_back(std::shared_ptr<Object>(obj_result.value().release()));
        }
        
        // Add objects to inventory using ranges
        for (auto& obj : test_objects) {
            auto result = actor.inventory().add_item(obj);
            REQUIRE(result.has_value());
        }
        
        REQUIRE(actor.inventory().item_count() == 10);
        
        // Use ranges to find specific items
        auto all_items = actor.inventory().get_all_items();
        
        // Find items matching criteria using ranges
        auto matching_items = all_items 
            | std::views::filter([](const auto& item) { 
                return item->name().find("TestItem") != std::string::npos; 
              })
            | std::views::take(5); // Only first 5
        
        size_t count = std::ranges::distance(matching_items);
        REQUIRE(count == 5);
        
        // Test range-based validation
        bool all_valid = std::ranges::all_of(all_items, [](const auto& item) {
            return item->validate().has_value();
        });
        REQUIRE(all_valid);
    }
}

/**
 * std::expected error handling patterns
 */
TEST_CASE("Modern Error Handling: std::expected Patterns", "[unit][error][expected]") {
    SECTION("Chaining operations with expected") {
        auto create_and_validate_player = [](EntityId id, std::string_view name) -> Result<std::unique_ptr<Player>> {
            auto player_result = Player::create(id, name);
            if (!player_result.has_value()) {
                return std::unexpected(player_result.error());
            }
            
            // Validate the created player
            auto validation_result = player_result.value()->validate();
            if (!validation_result.has_value()) {
                return std::unexpected(validation_result.error());
            }
            
            return std::move(player_result.value());
        };
        
        // Test successful case
        auto good_result = create_and_validate_player(EntityId{2001}, "ValidPlayer");
        REQUIRE(good_result.has_value());
        REQUIRE(good_result.value()->name() == "ValidPlayer");
        
        // Test failure case
        auto bad_result = create_and_validate_player(INVALID_ENTITY_ID, "");
        REQUIRE(!bad_result.has_value());
        REQUIRE(bad_result.error().code == ErrorCode::InvalidArgument);
    }
    
    SECTION("Error transformation and context") {
        auto divide_with_context = [](int a, int b, std::string_view context) -> Result<double> {
            if (b == 0) {
                return std::unexpected(Error{ErrorCode::InvalidArgument, 
                                           "Division by zero", 
                                           std::string(context)});
            }
            return static_cast<double>(a) / b;
        };
        
        auto result = divide_with_context(10, 0, "test_calculation");
        REQUIRE(!result.has_value());
        REQUIRE(result.error().context == "test_calculation");
        REQUIRE(result.error().message.find("Division by zero") != std::string::npos);
    }
}

/**
 * Concept-based testing for type safety
 */
template<ActorLike T>
void test_actor_basic_properties(const T& actor) {
    REQUIRE(actor.id().is_valid());
    REQUIRE(!actor.name().empty());
    REQUIRE(actor.stats().level >= 1);
    REQUIRE(actor.stats().max_hit_points > 0);
    REQUIRE(actor.is_alive());
}

TEST_CASE("Concepts: Type-Safe Actor Testing", "[unit][concepts][actor]") {
    SECTION("Player satisfies ActorLike concept") {
        auto player = Player::create(EntityId{3001}, "ConceptPlayer");
        REQUIRE(player.has_value());
        
        // This will only compile if Player satisfies ActorLike concept
        test_actor_basic_properties(*player.value());
        
        // Additional player-specific tests
        REQUIRE(player.value()->player_class() == "warrior"); // Default class
        REQUIRE(player.value()->race() == "human"); // Default race
    }
    
    SECTION("Mobile satisfies ActorLike concept") {
        auto mobile = Mobile::create(EntityId{3002}, "ConceptMobile", 5);
        REQUIRE(mobile.has_value());
        
        // This will only compile if Mobile satisfies ActorLike concept  
        test_actor_basic_properties(*mobile.value());
        
        // Additional mobile-specific tests
        REQUIRE(mobile.value()->stats().level == 5);
    }
}

/**
 * Benchmark testing for performance regressions
 */
TEST_CASE("Performance: Critical Path Benchmarking", "[unit][performance][benchmark]") {
    SECTION("Entity ID operations benchmark") {
        constexpr int iterations = 100000;
        
        auto start = std::chrono::high_resolution_clock::now();
        
        // Benchmark entity ID creation and comparison
        std::vector<EntityId> ids;
        ids.reserve(iterations);
        
        for (int i = 0; i < iterations; ++i) {
            ids.emplace_back(static_cast<uint64_t>(i));
        }
        
        // Benchmark lookup operations
        size_t found_count = 0;
        for (const auto& id : ids) {
            if (id.is_valid() && id.value() < iterations / 2) {
                ++found_count;
            }
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        
        // Performance requirement: Should complete in < 10ms
        REQUIRE(duration.count() < 10000);
        // EntityId(0) is invalid, so we expect (iterations / 2) - 1 valid IDs
        // IDs 1 to 49999 = 49999 IDs (not 50000 because 0 is invalid)
        REQUIRE(found_count == (iterations / 2) - 1);
        
        INFO("Entity ID operations: " << iterations << " iterations in " << duration.count() << "μs");
    }
    
    SECTION("Combat calculation benchmark") {
        constexpr int combat_rounds = 1000;
        
        auto attacker = Player::create(EntityId{4001}, "BenchAttacker");
        auto target = Player::create(EntityId{4002}, "BenchTarget");
        REQUIRE(attacker.has_value());
        REQUIRE(target.has_value());
        
        // Set up guaranteed hits: high level attacker vs low AC target
        attacker.value()->stats().level = 10;      // +10 to hit
        attacker.value()->stats().strength = 18;   // Better damage
        attacker.value()->stats().hit_roll = 5;    // +5 to hit bonus
        target.value()->stats().armor_class = 5;   // Very low AC (easier to hit)
        
        // Convert to shared_ptr as expected by combat system
        auto attacker_ptr = std::shared_ptr<Actor>(attacker.value().release());
        auto target_ptr = std::shared_ptr<Actor>(target.value().release());
        
        auto start = std::chrono::high_resolution_clock::now();
        
        for (int i = 0; i < combat_rounds; ++i) {
            auto result = FieryMUD::CombatSystem::perform_attack(attacker_ptr, target_ptr);
            REQUIRE(result.type != FieryMUD::CombatResult::Type::Miss);
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        // Performance requirement: 1000 combat calculations in < 50ms
        REQUIRE(duration.count() < 50);
        
        INFO("Combat calculations: " << combat_rounds << " rounds in " << duration.count() << "ms");
    }
}