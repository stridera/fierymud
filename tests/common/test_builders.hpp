#pragma once

#include "../../src/core/actor.hpp"
#include "../../src/core/object.hpp"
#include "../../src/world/room.hpp"
#include <memory>
#include <string>
#include <vector>
#include <random>
#include <algorithm>
#include <optional>

/**
 * Test Data Builders - Modern C++ approach to test fixture creation
 * 
 * Benefits:
 * - Fluent, readable test setup
 * - Sensible defaults with customization options
 * - Type-safe construction
 * - Reusable across tests
 * - Easy to extend and modify
 */

/**
 * PlayerBuilder - Fluent interface for creating test players
 */
class PlayerBuilder {
public:
    PlayerBuilder() {
        id_ = EntityId{next_id_++};
        name_ = "TestPlayer";
        level_ = 1;
        character_class_ = "warrior";
        race_ = "human";
    }
    
    PlayerBuilder& with_id(EntityId id) { id_ = id; return *this; }
    PlayerBuilder& named(std::string_view name) { name_ = name; return *this; }
    PlayerBuilder& at_level(int level) { level_ = level; return *this; }
    PlayerBuilder& of_class(std::string_view cls) { character_class_ = cls; return *this; }
    PlayerBuilder& of_race(std::string_view race) { race_ = race; return *this; }
    
    PlayerBuilder& with_strength(int str) { strength_ = str; return *this; }
    PlayerBuilder& with_dexterity(int dex) { dexterity_ = dex; return *this; }
    PlayerBuilder& with_intelligence(int intel) { intelligence_ = intel; return *this; }
    PlayerBuilder& with_constitution(int con) { constitution_ = con; return *this; }
    
    PlayerBuilder& with_hp(int hp) { hit_points_ = hp; return *this; }
    PlayerBuilder& with_max_hp(int max_hp) { max_hit_points_ = max_hp; return *this; }
    
    PlayerBuilder& as_warrior() {
        return of_class("warrior").with_strength(16).with_hp(80);
    }
    
    PlayerBuilder& as_sorcerer() {
        return of_class("sorcerer").with_intelligence(16).with_hp(50);
    }
    
    PlayerBuilder& as_rogue() {
        return of_class("rogue").with_dexterity(16).with_hp(60);
    }
    
    PlayerBuilder& as_cleric() {
        return of_class("cleric").with_constitution(15).with_hp(70);
    }
    
    std::unique_ptr<Player> build() {
        auto result = Player::create(id_, name_);
        if (!result.has_value()) {
            throw std::runtime_error("Failed to create player: " + result.error().message);
        }
        
        auto player = std::move(result.value());
        
        // Set basic properties
        player->set_class(character_class_);
        player->set_race(race_);
        
        // Set stats
        auto& stats = player->stats();
        stats.level = level_;
        
        if (strength_) stats.strength = *strength_;
        if (dexterity_) stats.dexterity = *dexterity_;
        if (intelligence_) stats.intelligence = *intelligence_;
        if (constitution_) stats.constitution = *constitution_;
        
        if (hit_points_) stats.hit_points = *hit_points_;
        if (max_hit_points_) stats.max_hit_points = *max_hit_points_;
        
        return player;
    }
    
private:
    static inline uint64_t next_id_ = 10000;
    
    EntityId id_;
    std::string name_;
    int level_;
    std::string character_class_;
    std::string race_;
    
    std::optional<int> strength_;
    std::optional<int> dexterity_;
    std::optional<int> intelligence_;
    std::optional<int> constitution_;
    std::optional<int> hit_points_;
    std::optional<int> max_hit_points_;
};

/**
 * ObjectBuilder - Fluent interface for creating test objects
 */
class ObjectBuilder {
public:
    ObjectBuilder() {
        id_ = EntityId{next_id_++};
        name_ = "TestObject";
        type_ = ObjectType::Other;
    }
    
    ObjectBuilder& with_id(EntityId id) { id_ = id; return *this; }
    ObjectBuilder& named(std::string_view name) { name_ = name; return *this; }
    ObjectBuilder& of_type(ObjectType type) { type_ = type; return *this; }
    ObjectBuilder& with_weight(int weight) { weight_ = weight; return *this; }
    ObjectBuilder& with_value(int value) { value_ = value; return *this; }
    ObjectBuilder& with_description(std::string_view desc) { description_ = desc; return *this; }
    
    // Convenience methods for common object types
    ObjectBuilder& as_weapon(int damage = 10) {
        return of_type(ObjectType::Weapon).with_value(damage * 5);
    }
    
    ObjectBuilder& as_armor(int ac_bonus = 2) {
        return of_type(ObjectType::Armor).with_value(ac_bonus * 10);
    }
    
    ObjectBuilder& as_container(int capacity = 100) {
        return of_type(ObjectType::Container).with_weight(5).with_value(capacity);
    }
    
    ObjectBuilder& as_treasure(int gold_value) {
        return of_type(ObjectType::Treasure).with_value(gold_value).with_weight(1);
    }
    
    std::unique_ptr<Object> build() {
        auto result = Object::create(id_, name_, type_);
        if (!result.has_value()) {
            throw std::runtime_error("Failed to create object: " + result.error().message);
        }
        
        auto object = std::move(result.value());
        
        if (weight_) object->set_weight(*weight_);
        if (value_) object->set_value(*value_);
        if (description_) object->set_description(*description_);
        
        return object;
    }
    
private:
    static inline uint64_t next_id_ = 20000;
    
    EntityId id_;
    std::string name_;
    ObjectType type_;
    std::optional<int> weight_;
    std::optional<int> value_;
    std::optional<std::string> description_;
};

/**
 * RoomBuilder - Fluent interface for creating test rooms
 */
class RoomBuilder {
public:
    RoomBuilder() {
        id_ = EntityId{next_id_++};
        name_ = "Test Room";
        sector_ = SectorType::Inside;
    }
    
    RoomBuilder& with_id(EntityId id) { id_ = id; return *this; }
    RoomBuilder& named(std::string_view name) { name_ = name; return *this; }
    RoomBuilder& with_sector(SectorType sector) { sector_ = sector; return *this; }
    RoomBuilder& with_description(std::string_view desc) { description_ = desc; return *this; }
    
    RoomBuilder& with_exit(std::string_view direction, EntityId target_room, 
                          bool closed = false, bool locked = false) {
        exits_.emplace_back(TestExitInfo{std::string(direction), target_room, closed, locked});
        return *this;
    }
    
    // Convenience methods for common room types
    RoomBuilder& as_outdoor() {
        return with_sector(SectorType::Field)
               .with_description("This is an outdoor area with open sky above.");
    }
    
    RoomBuilder& as_dungeon_room() {
        return with_sector(SectorType::Inside)
               .with_description("A dark, stone-walled chamber deep underground.");
    }
    
    RoomBuilder& as_safe_room() {
        return named("Safe Room")
               .with_description("This room emanates a feeling of safety and peace.");
    }
    
    std::unique_ptr<Room> build() {
        auto result = Room::create(id_, name_, sector_);
        if (!result.has_value()) {
            throw std::runtime_error("Failed to create room: " + result.error().message);
        }
        
        auto room = std::move(result.value());
        
        if (description_) {
            room->set_description(*description_);
        }
        
        // Add exits using proper ExitInfo structure
        for (const auto& exit : exits_) {
            ExitInfo exit_info;
            exit_info.to_room = exit.target_room;
            // Convert string direction to Direction enum
            if (auto dir = parse_direction(exit.direction)) {
                room->set_exit(*dir, exit_info);
            }
        }
        
        return room;
    }
    
private:
    // Helper function to parse direction strings
    std::optional<Direction> parse_direction(const std::string& dir_str) const {
        std::string lower_str = dir_str;
        std::transform(lower_str.begin(), lower_str.end(), lower_str.begin(), ::tolower);
        
        if (lower_str == "north" || lower_str == "n") return Direction::North;
        if (lower_str == "south" || lower_str == "s") return Direction::South;
        if (lower_str == "east" || lower_str == "e") return Direction::East;
        if (lower_str == "west" || lower_str == "w") return Direction::West;
        if (lower_str == "up" || lower_str == "u") return Direction::Up;
        if (lower_str == "down" || lower_str == "d") return Direction::Down;
        
        return std::nullopt;
    }
    struct TestExitInfo {
        std::string direction;
        EntityId target_room;
        bool closed;
        bool locked;
    };
    
    static inline uint64_t next_id_ = 30000;
    
    EntityId id_;
    std::string name_;
    SectorType sector_;
    std::optional<std::string> description_;
    std::vector<TestExitInfo> exits_;
};

/**
 * ScenarioBuilder - Creates complete test scenarios
 */
class ScenarioBuilder {
public:
    /**
     * Create a basic combat scenario with two players
     */
    struct CombatScenario {
        std::unique_ptr<Player> attacker;
        std::unique_ptr<Player> target;
        std::unique_ptr<Room> combat_room;
    };
    
    static CombatScenario create_basic_combat() {
        auto room = RoomBuilder()
            .named("Combat Arena")
            .as_dungeon_room()
            .build();
        
        auto attacker = PlayerBuilder()
            .named("Attacker")
            .as_warrior()
            .at_level(10)
            .build();
            
        auto target = PlayerBuilder()
            .named("Target")
            .as_sorcerer()
            .at_level(8)
            .build();
        
        return CombatScenario{
            std::move(attacker),
            std::move(target), 
            std::move(room)
        };
    }
    
    /**
     * Create a treasure hunting scenario
     */
    struct TreasureScenario {
        std::unique_ptr<Player> player;
        std::unique_ptr<Room> treasure_room;
        std::vector<std::unique_ptr<Object>> treasures;
    };
    
    static TreasureScenario create_treasure_hunt() {
        auto room = RoomBuilder()
            .named("Treasure Chamber")
            .with_description("Ancient treasures glitter in the torchlight.")
            .as_dungeon_room()
            .build();
        
        auto player = PlayerBuilder()
            .named("TreasureHunter")
            .as_rogue()
            .at_level(5)
            .build();
        
        std::vector<std::unique_ptr<Object>> treasures;
        treasures.push_back(ObjectBuilder().named("Golden Crown").as_treasure(1000).build());
        treasures.push_back(ObjectBuilder().named("Silver Chalice").as_treasure(500).build());
        treasures.push_back(ObjectBuilder().named("Gem-encrusted Dagger").as_weapon(15).build());
        
        return TreasureScenario{
            std::move(player),
            std::move(room),
            std::move(treasures)
        };
    }
};

/**
 * RandomDataGenerator - For property-based testing
 */
class RandomDataGenerator {
public:
    explicit RandomDataGenerator(uint32_t seed = std::random_device{}()) 
        : rng_(seed) {}
    
    // Generate random player with realistic stats
    std::unique_ptr<Player> random_player() {
        auto classes = std::vector<std::string>{"warrior", "cleric", "sorcerer", "rogue"};
        auto races = std::vector<std::string>{"human", "elf", "dwarf", "halfling"};
        
        std::uniform_int_distribution<> level_dist(1, 20);
        std::uniform_int_distribution<> stat_dist(8, 18);
        std::uniform_int_distribution<> class_dist(0, classes.size() - 1);
        std::uniform_int_distribution<> race_dist(0, races.size() - 1);
        
        return PlayerBuilder()
            .named(generate_random_name())
            .at_level(level_dist(rng_))
            .of_class(classes[class_dist(rng_)])
            .of_race(races[race_dist(rng_)])
            .with_strength(stat_dist(rng_))
            .with_dexterity(stat_dist(rng_))
            .with_intelligence(stat_dist(rng_))
            .with_constitution(stat_dist(rng_))
            .build();
    }
    
    // Generate random object
    std::unique_ptr<Object> random_object() {
        auto types = std::vector<ObjectType>{
            ObjectType::Weapon, ObjectType::Armor, ObjectType::Treasure, 
            ObjectType::Container, ObjectType::Other
        };
        
        std::uniform_int_distribution<> type_dist(0, types.size() - 1);
        std::uniform_int_distribution<> value_dist(1, 1000);
        std::uniform_int_distribution<> weight_dist(1, 50);
        
        return ObjectBuilder()
            .named(generate_random_name() + " Item")
            .of_type(types[type_dist(rng_)])
            .with_value(value_dist(rng_))
            .with_weight(weight_dist(rng_))
            .build();
    }
    
private:
    std::mt19937 rng_;
    
    std::string generate_random_name() {
        auto prefixes = std::vector<std::string>{"Test", "Random", "Auto", "Gen"};
        auto suffixes = std::vector<std::string>{"Player", "Actor", "Hero", "Character"};
        
        std::uniform_int_distribution<> prefix_dist(0, prefixes.size() - 1);
        std::uniform_int_distribution<> suffix_dist(0, suffixes.size() - 1);
        std::uniform_int_distribution<> num_dist(1000, 9999);
        
        return prefixes[prefix_dist(rng_)] + suffixes[suffix_dist(rng_)] + 
               std::to_string(num_dist(rng_));
    }
};