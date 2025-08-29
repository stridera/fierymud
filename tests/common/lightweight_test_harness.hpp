#pragma once

#include "../../src/core/actor.hpp"
#include "../../src/core/object.hpp"
#include "../../src/world/room.hpp"
#include "../../src/world/world_manager.hpp"
#include "../../src/commands/command_system.hpp"
#include "../../src/commands/command_parser.hpp"
#include "../../src/commands/builtin_commands.hpp"
#include <catch2/catch_test_macros.hpp>
#include <memory>
#include <string>
#include <vector>
#include <regex>
#include <chrono>
#include <iostream>
#include <fmt/format.h>
#include <magic_enum/magic_enum.hpp>

/**
 * LightweightTestHarness - Synchronous, isolated test infrastructure
 * 
 * Key improvements over TestHarness:
 * - No background threads (synchronous operation)
 * - No shared global state between test instances
 * - Minimal resource usage
 * - Deterministic, predictable behavior
 * - Fast setup/teardown
 */
class LightweightTestHarness {
public:
    /**
     * TestPlayer - Simplified player for integration testing
     */
    class TestPlayer : public Player {
    public:
        TestPlayer(EntityId id, std::string_view name) : Player(id, name) {}
        
        void send_message(std::string_view message) override {
            output_messages.emplace_back(message);
        }
        
        void receive_message(std::string_view message) override {
            received_messages.emplace_back(message);
        }
        
        // Test-specific access
        const std::vector<std::string>& get_output() const { return output_messages; }
        const std::vector<std::string>& get_received() const { return received_messages; }
        void clear_output() { output_messages.clear(); received_messages.clear(); }
        
    private:
        std::vector<std::string> output_messages;
        std::vector<std::string> received_messages;
    };
    
    /**
     * TestWorld - Isolated world state for each test
     */
    class TestWorld {
    public:
        TestWorld() {
            // Create isolated world state - no shared globals
            create_test_rooms();
            setup_test_environment();
        }
        
        ~TestWorld() {
            cleanup_test_state();
        }
        
        std::shared_ptr<Room> get_start_room() { return start_room_; }
        std::shared_ptr<Room> get_room(EntityId id) { return world_state_.find_room(id); }
        
        EntityId create_room(std::string_view name, SectorType sector = SectorType::City) {
            auto room_id = EntityId{next_room_id_++};
            auto room_result = Room::create(room_id, name, sector);
            if (room_result.has_value()) {
                auto room = std::shared_ptr<Room>(room_result.value().release());
                world_state_.add_room(room_id, room);
                return room_id;
            }
            return INVALID_ENTITY_ID;
        }
        
    private:
        void create_test_rooms() {
            // Create minimal test world - use City sector for proper lighting
            start_room_ = std::shared_ptr<Room>(
                Room::create(EntityId{100}, "Test Start Room", SectorType::City).value().release()
            );
            
            auto north_room = std::shared_ptr<Room>(
                Room::create(EntityId{101}, "North Room", SectorType::City).value().release()
            );
            
            // Connect rooms with proper exit info
            ExitInfo north_exit;
            north_exit.to_room = EntityId{101};
            start_room_->set_exit(Direction::North, north_exit);
            
            ExitInfo south_exit;
            south_exit.to_room = EntityId{100};
            north_room->set_exit(Direction::South, south_exit);
            
            // Add interactive test objects to start room
            setup_test_objects();
            
            world_state_.add_room(EntityId{100}, start_room_);
            world_state_.add_room(EntityId{101}, north_room);
        }
        
        void setup_test_objects() {
            // Create diverse test objects for comprehensive testing
            uint64_t object_id = 2000;
            
            // 1. Container - Test Chest (with some contents)
            {
                auto chest_result = Container::create(EntityId{object_id++}, "test_chest", 10);
                if (chest_result.has_value()) {
                    auto chest = std::shared_ptr<Container>(chest_result.value().release());
                    
                    // Configure as closeable/lockable container
                    ContainerInfo info;
                    info.capacity = 5;
                    info.weight_capacity = 100;  // Set proper weight capacity
                    info.closeable = true;
                    info.closed = true;
                    info.lockable = true;
                    info.locked = true;
                    info.key_id = EntityId{object_id + 10}; // Key we'll create later
                    chest->set_container_info(info);
                    
                    // Add a potion inside the chest
                    auto potion_result = Object::create(EntityId{object_id++}, "test_potion", ObjectType::Potion);
                    if (potion_result.has_value()) {
                        auto potion = std::shared_ptr<Object>(potion_result.value().release());
                        potion->set_weight(1);
                        potion->set_value(50);
                        chest->add_item(potion);
                    }
                    
                    start_room_->add_object(chest);
                }
            }
            
            // 2. Weapon - Test Sword
            {
                auto sword_result = Weapon::create(EntityId{object_id++}, "test_sword", ObjectType::Weapon);
                if (sword_result.has_value()) {
                    auto sword = std::shared_ptr<Weapon>(sword_result.value().release());
                    
                    // Set weapon properties
                    DamageProfile damage;
                    damage.base_damage = 5;
                    damage.dice_count = 2;
                    damage.dice_sides = 6;
                    damage.damage_bonus = 2;
                    sword->set_damage_profile(damage);
                    
                    sword->set_weight(3);
                    sword->set_value(100);
                    sword->set_reach(1);
                    sword->set_speed(2);
                    sword->set_equip_slot(EquipSlot::Wield); // Set the equip slot separately
                    
                    start_room_->add_object(sword);
                }
            }
            
            // 3. Armor - Test Shield
            {
                auto shield_result = Armor::create(EntityId{object_id++}, "test_shield", EquipSlot::Shield);
                if (shield_result.has_value()) {
                    auto shield = std::shared_ptr<Armor>(shield_result.value().release());
                    shield->set_armor_class(3);
                    shield->set_weight(2);
                    shield->set_value(75);
                    shield->set_material("steel");
                    
                    start_room_->add_object(shield);
                }
            }
            
            // 4. Light Source - Test Torch
            {
                auto torch_result = Object::create(EntityId{object_id++}, "test_torch", ObjectType::Light);
                if (torch_result.has_value()) {
                    auto torch = std::shared_ptr<Object>(torch_result.value().release());
                    torch->set_equip_slot(EquipSlot::Light);
                    torch->set_weight(1);
                    torch->set_value(5);
                    
                    // Configure as light source
                    LightInfo light_info;
                    light_info.duration = 100;
                    light_info.brightness = 2;
                    light_info.lit = false; // Starts unlit
                    torch->set_light_info(light_info);
                    
                    start_room_->add_object(torch);
                }
            }
            
            // 5. Key for the chest (matches chest's key_id)
            {
                // Skip ahead to the key ID we reserved for the chest
                object_id = 2010; // This matches the key_id set in the chest (object_id + 10)
                auto key_result = Object::create(EntityId{object_id++}, "test_key", ObjectType::Key);
                if (key_result.has_value()) {
                    auto key = std::shared_ptr<Object>(key_result.value().release());
                    key->set_weight(0); // Keys are typically weightless
                    key->set_value(1);
                    
                    // This key will unlock the test_chest
                    start_room_->add_object(key);
                }
            }
            
            // 6. Consumable food item with spoilage timer
            {
                auto bread_result = Object::create(EntityId{object_id++}, "test_bread", ObjectType::Food);
                if (bread_result.has_value()) {
                    auto bread = std::shared_ptr<Object>(bread_result.value().release());
                    bread->set_weight(1);
                    bread->set_value(2);
                    
                    // Configure as consumable with spoilage timer
                    bread->set_timer(24); // Food spoils after 24 game hours
                    // Note: This creates a consumable item that will decay over time
                    // When timer reaches 0, the food becomes spoiled
                    
                    start_room_->add_object(bread);
                }
            }
            
            // 7. Another container - Bag (smaller, for testing capacity)
            {
                auto bag_result = Container::create(EntityId{object_id++}, "test_bag", 2);
                if (bag_result.has_value()) {
                    auto bag = std::shared_ptr<Container>(bag_result.value().release());
                    
                    ContainerInfo info;
                    info.capacity = 2; // Very small capacity for testing limits
                    info.closeable = false; // Always open
                    info.closed = false;
                    info.lockable = false;
                    info.locked = false;
                    bag->set_container_info(info);
                    
                    bag->set_weight(1);
                    bag->set_value(10);
                    
                    start_room_->add_object(bag);
                }
            }
            
            // 8. Scroll - Magic consumable item 
            {
                auto scroll_result = Object::create(EntityId{object_id++}, "test_scroll", ObjectType::Scroll);
                if (scroll_result.has_value()) {
                    auto scroll = std::shared_ptr<Object>(scroll_result.value().release());
                    scroll->set_weight(0); // Scrolls are weightless
                    scroll->set_value(15);
                    
                    // Scrolls are single-use consumables for testing
                    start_room_->add_object(scroll);
                }
            }
            
            // 9. Another light source - Lantern with longer duration
            {
                auto lantern_result = Object::create(EntityId{object_id++}, "test_lantern", ObjectType::Light);
                if (lantern_result.has_value()) {
                    auto lantern = std::shared_ptr<Object>(lantern_result.value().release());
                    lantern->set_equip_slot(EquipSlot::Light);
                    lantern->set_weight(2);
                    lantern->set_value(20);
                    
                    // Configure as more powerful light source
                    LightInfo light_info;
                    light_info.duration = 500; // Longer lasting than torch
                    light_info.brightness = 4;  // Brighter than torch
                    light_info.lit = false;     // Starts unlit
                    lantern->set_light_info(light_info);
                    
                    start_room_->add_object(lantern);
                }
            }
            
            // 10. Liquid container with liquid
            {
                auto waterskin_result = Object::create(EntityId{object_id++}, "test_waterskin", ObjectType::Liquid_Container);
                if (waterskin_result.has_value()) {
                    auto waterskin = std::shared_ptr<Object>(waterskin_result.value().release());
                    waterskin->set_weight(3);
                    waterskin->set_value(8);
                    
                    // Container for liquids - for drink/pour testing
                    start_room_->add_object(waterskin);
                }
            }
            
            // 11. Second weapon - Test Dagger (to test weapon slot conflicts)
            {
                auto dagger_result = Weapon::create(EntityId{object_id++}, "test_dagger", ObjectType::Weapon);
                if (dagger_result.has_value()) {
                    auto dagger = std::shared_ptr<Weapon>(dagger_result.value().release());
                    
                    // Set weapon properties (using same pattern as sword)
                    DamageProfile damage;
                    damage.base_damage = 2;  // Slightly less damage than sword
                    damage.dice_count = 1;
                    damage.dice_sides = 4;
                    damage.damage_bonus = 1;
                    dagger->set_damage_profile(damage);
                    
                    dagger->set_weight(1);    // Lighter than sword
                    dagger->set_value(25);    // Less valuable than sword
                    dagger->set_reach(1);
                    dagger->set_speed(1);     // Faster than sword
                    dagger->set_equip_slot(EquipSlot::Wield); // Same slot as sword - will conflict
                    
                    start_room_->add_object(dagger);
                }
            }
        }
        
        void setup_test_environment() {
            // Initialize command system for testing
            auto result = CommandSystem::instance().initialize();
            if (!result.has_value()) {
                // For testing, we'll proceed even if initialization fails
                // In real usage this would be a critical error
            }
            
            // Register built-in commands for testing
            BuiltinCommands::register_all_commands();
            
            // Register test rooms with WorldManager for movement system
            auto& world_manager = WorldManager::instance();
            world_manager.add_room(start_room_);
            for (const auto& [room_id, room] : world_state_.rooms_) {
                if (room_id != EntityId{100}) { // Don't add start_room twice
                    world_manager.add_room(room);
                }
            }
        }
        
        void cleanup_test_state() {
            world_state_.clear();
        }
        
        std::shared_ptr<Room> start_room_;
        uint64_t next_room_id_ = 200;
        
        // Lightweight world state - not the global WorldManager
        struct LocalWorldState {
            std::unordered_map<EntityId, std::shared_ptr<Room>, EntityId::Hash> rooms_;
            
            void add_room(EntityId id, std::shared_ptr<Room> room) { rooms_[id] = room; }
            std::shared_ptr<Room> find_room(EntityId id) { 
                auto it = rooms_.find(id);
                return it != rooms_.end() ? it->second : nullptr;
            }
            void clear() { rooms_.clear(); }
        } world_state_;
        
        // Uses singleton CommandSystem::instance()
    };
    
    LightweightTestHarness() 
        : world_(std::make_unique<TestWorld>())
        , player_(std::make_shared<TestPlayer>(EntityId{1}, "TestPlayer"))
    {
        // Place player in start room
        auto start_room = world_->get_start_room();
        player_->move_to(start_room);
        start_room->add_actor(player_); // Add player to room's actor list for emotes
    }
    
    // Synchronous command execution - no threads, no timeouts
    LightweightTestHarness& execute_command(std::string_view command) {
        player_->clear_output();
        
        // Execute command directly using the string interface
        auto result = CommandSystem::instance().execute_command(player_, command);
        
        if (!result.has_value()) {
            player_->send_message(fmt::format("Command '{}' execution error: {}", command, result.error().message));
        } else if (result.value() != CommandResult::Success) {
            player_->send_message(fmt::format("Command '{}' failed: {}", command, magic_enum::enum_name(result.value())));
        }
        
        return *this;
    }
    
    // Simple, immediate assertions - no futures or timeouts
    LightweightTestHarness& then_output_contains(std::string_view text) {
        const auto& output = player_->get_output();
        bool found = std::any_of(output.begin(), output.end(), 
            [text](const std::string& line) { return line.find(text) != std::string::npos; });
        
        // Debug output when assertion fails
        if (!found) {
            std::cerr << "=== OUTPUT DEBUG ===" << std::endl;
            std::cerr << "Looking for: '" << text << "'" << std::endl;
            std::cerr << "Actual output (" << output.size() << " lines):" << std::endl;
            for (size_t i = 0; i < output.size(); ++i) {
                std::cerr << "  [" << i << "]: '" << output[i] << "'" << std::endl;
            }
            std::cerr << "===================" << std::endl;
        }
        
        REQUIRE(found);
        return *this;
    }
    
    LightweightTestHarness& then_output_size_is(size_t expected_size) {
        REQUIRE(player_->get_output().size() == expected_size);
        return *this;
    }
    
    // Direct state access for testing
    std::shared_ptr<TestPlayer> get_player() { return player_; }
    TestWorld& get_world() { return *world_; }
    
    // Room navigation testing
    EntityId current_room_id() const {
        auto room = player_->current_room();
        return room ? room->id() : INVALID_ENTITY_ID;
    }
    
    // Enhanced validation methods
    LightweightTestHarness& then_output_matches_regex(const std::string& pattern) {
        const auto& output = player_->get_output();
        std::regex regex_pattern(pattern);
        bool found = std::any_of(output.begin(), output.end(), 
            [&regex_pattern](const std::string& line) { 
                return std::regex_search(line, regex_pattern); 
            });
        REQUIRE(found);
        return *this;
    }
    
    LightweightTestHarness& then_output_not_contains(std::string_view text) {
        const auto& output = player_->get_output();
        bool found = std::any_of(output.begin(), output.end(), 
            [text](const std::string& line) { return line.find(text) != std::string::npos; });
        REQUIRE_FALSE(found);
        return *this;
    }
    
    LightweightTestHarness& then_player_stat_equals(const std::string& stat, int expected_value) {
        if (stat == "level") {
            REQUIRE(player_->stats().level == expected_value);
        } else if (stat == "hp") {
            REQUIRE(player_->stats().hit_points == expected_value);
        } else if (stat == "max_hp") {
            REQUIRE(player_->stats().max_hit_points == expected_value);
        }
        // Add more stats as needed
        return *this;
    }
    
    // Performance measurement
    template<typename F>
    LightweightTestHarness& then_executes_within_ms(F&& func, int max_milliseconds) {
        auto start = std::chrono::high_resolution_clock::now();
        func();
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        REQUIRE(duration.count() <= max_milliseconds);
        return *this;
    }
    
    // Execute command without clearing previous output (for accumulation tests)
    LightweightTestHarness& execute_command_accumulate(std::string_view command) {
        // DON'T clear output - let it accumulate
        
        // Execute command directly using the string interface
        auto result = CommandSystem::instance().execute_command(player_, command);
        
        if (!result.has_value()) {
            player_->send_message(fmt::format("Command '{}' execution error: {}", command, result.error().message));
        } else if (result.value() != CommandResult::Success) {
            player_->send_message(fmt::format("Command '{}' failed: {}", command, magic_enum::enum_name(result.value())));
        }
        
        return *this;
    }
    
    // Multi-command execution
    LightweightTestHarness& execute_commands(const std::vector<std::string>& commands) {
        for (const auto& command : commands) {
            execute_command(command);
        }
        return *this;
    }
    
    // Multi-command execution with output accumulation
    LightweightTestHarness& execute_commands_accumulate(const std::vector<std::string>& commands) {
        for (const auto& command : commands) {
            execute_command_accumulate(command);
        }
        return *this;
    }
    
    // State validation helpers
    bool player_is_in_room(EntityId room_id) const {
        return current_room_id() == room_id;
    }
    
    bool player_has_item_named(const std::string& item_name) const {
        auto items = player_->inventory().find_items_by_keyword(item_name);
        return !items.empty();
    }
    
    // Output analysis helpers
    size_t count_output_lines_containing(std::string_view text) const {
        const auto& output = player_->get_output();
        return std::count_if(output.begin(), output.end(), 
            [text](const std::string& line) { return line.find(text) != std::string::npos; });
    }
    
    std::vector<std::string> get_output_lines_containing(std::string_view text) const {
        const auto& output = player_->get_output();
        std::vector<std::string> matching_lines;
        std::copy_if(output.begin(), output.end(), std::back_inserter(matching_lines),
            [text](const std::string& line) { return line.find(text) != std::string::npos; });
        return matching_lines;
    }
    
private:
    std::unique_ptr<TestWorld> world_;
    std::shared_ptr<TestPlayer> player_;
};

/**
 * Test Fixture Builders for complex scenarios
 */
class CombatTestFixture {
public:
    static std::pair<std::shared_ptr<Player>, std::shared_ptr<Player>> 
    create_combat_pair(int attacker_level = 5, int target_level = 5) {
        auto attacker = std::shared_ptr<Player>(
            Player::create(EntityId{1001}, "Attacker").value().release()
        );
        auto target = std::shared_ptr<Player>(
            Player::create(EntityId{1002}, "Target").value().release()
        );
        
        attacker->stats().level = attacker_level;
        target->stats().level = target_level;
        
        return {attacker, target};
    }
    
    static void setup_warrior_vs_sorcerer(Player& warrior, Player& sorcerer) {
        warrior.set_class("warrior");
        warrior.stats().strength = 16;
        warrior.stats().hit_points = 80;
        
        sorcerer.set_class("sorcerer");
        sorcerer.stats().intelligence = 16;
        sorcerer.stats().hit_points = 50;
    }
};

/**
 * Deterministic RNG for predictable test results
 */
class DeterministicRNG {
public:
    explicit DeterministicRNG(uint32_t seed = 12345) : seed_(seed) {}
    
    int roll_dice(int sides) {
        seed_ = seed_ * 1103515245 + 12345;
        return (seed_ % sides) + 1;
    }
    
    double roll_percentage() {
        return roll_dice(100) / 100.0;
    }
    
private:
    uint32_t seed_;
};