#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include <memory>
#include <string>

#include "../src/core/object.hpp"
#include "../src/core/actor.hpp"
#include "../src/world/room.hpp"

TEST_CASE("Object Interaction Integration Test", "[integration][objects][commands]") {
    
    // Create a test room
    auto room_result = Room::create(EntityId{1001}, "Test Room");
    REQUIRE(room_result.has_value());
    auto room = std::shared_ptr<Room>(room_result.value().release());
    room->set_short_description("a test room");
    room->set_description("This is a test room for object interactions.");
    
    // Create a test player
    auto player_result = Player::create(EntityId{2001}, "TestPlayer");
    REQUIRE(player_result.has_value());
    auto player = std::shared_ptr<Player>(player_result.value().release());
    player->set_current_room(room);
    
    // Create test objects with different keyword patterns
    auto bread = Object::create(EntityId{3010}, "bread loaf food", ObjectType::Food);
    REQUIRE(bread.has_value());
    (*bread)->set_short_description("some bread");
    (*bread)->set_description("A fresh loaf of bread with a golden crust.");
    
    auto sword = Object::create(EntityId{3020}, "sword blade weapon", ObjectType::Weapon);
    REQUIRE(sword.has_value());
    (*sword)->set_short_description("a sharp sword");
    (*sword)->set_description("A well-forged steel sword with a gleaming blade.");
    
    // Add objects to the room
    room->add_object(std::shared_ptr<Object>(bread.value().release()));
    room->add_object(std::shared_ptr<Object>(sword.value().release()));
    room->add_actor(player);
    
    SECTION("Look command shows objects with short descriptions") {
        // Simulate 'look' command
        std::ostringstream output;
        
        // Test that we can see objects in room with their short descriptions
        auto room_objects = room->contents().objects;
        REQUIRE(room_objects.size() == 2);
        
        // Verify bread object
        bool found_bread = false;
        bool found_sword = false;
        
        for (const auto& obj : room_objects) {
            if (obj->id() == EntityId{3010}) {
                found_bread = true;
                REQUIRE(obj->short_description() == "some bread");
                REQUIRE(obj->name() == "bread loaf food");
                REQUIRE(obj->matches_keyword("bread"));
                REQUIRE(obj->matches_keyword("loaf"));
                REQUIRE(obj->matches_keyword("food"));
            }
            if (obj->id() == EntityId{3020}) {
                found_sword = true;
                REQUIRE(obj->short_description() == "a sharp sword");
                REQUIRE(obj->name() == "sword blade weapon");
                REQUIRE(obj->matches_keyword("sword"));
                REQUIRE(obj->matches_keyword("blade"));
                REQUIRE(obj->matches_keyword("weapon"));
            }
        }
        
        REQUIRE(found_bread);
        REQUIRE(found_sword);
    }
    
    SECTION("Object keyword matching works correctly") {
        auto room_objects = room->contents().objects;
        
        // Test bread keywords
        auto bread_obj = room_objects[0]->id() == EntityId{3010} ? room_objects[0] : room_objects[1];
        REQUIRE(bread_obj->matches_keyword("bread"));
        REQUIRE(bread_obj->matches_keyword("loaf"));
        REQUIRE(bread_obj->matches_keyword("food"));
        REQUIRE_FALSE(bread_obj->matches_keyword("sword"));
        REQUIRE_FALSE(bread_obj->matches_keyword("nonexistent"));
        
        // Test case insensitive matching
        REQUIRE(bread_obj->matches_keyword("BREAD"));
        REQUIRE(bread_obj->matches_keyword("Loaf"));
        REQUIRE(bread_obj->matches_keyword("FooD"));
    }
    
    SECTION("Take and drop commands work with keywords") {
        // Verify objects start in room
        REQUIRE(room->contents().objects.size() == 2);
        REQUIRE(player->inventory().empty());
        
        // Test finding objects by different keywords using room methods
        
        // Find bread using "bread" keyword
        auto bread_results = room->find_objects_by_keyword("bread");
        REQUIRE(bread_results.size() == 1);
        REQUIRE(bread_results[0]->id() == EntityId{3010});
        
        // Find sword using "blade" keyword
        auto blade_results = room->find_objects_by_keyword("blade");
        REQUIRE(blade_results.size() == 1);
        REQUIRE(blade_results[0]->id() == EntityId{3020});
        
        // Test finding using "weapon" keyword (should find sword)
        auto weapon_results = room->find_objects_by_keyword("weapon");
        REQUIRE(weapon_results.size() == 1);
        REQUIRE(weapon_results[0]->id() == EntityId{3020});
        
        // Test finding using "food" keyword (should find bread)
        auto food_results = room->find_objects_by_keyword("food");
        REQUIRE(food_results.size() == 1);
        REQUIRE(food_results[0]->id() == EntityId{3010});
        
        // Test non-existent keyword
        auto nonexistent_results = room->find_objects_by_keyword("nonexistent");
        REQUIRE(nonexistent_results.empty());
    }
    
    SECTION("Take and drop commands would work with keywords") {
        // This section validates that the system supports the functionality
        // needed for take/drop commands to work with keywords
        
        // Verify objects start in room
        REQUIRE(room->contents().objects.size() == 2);
        REQUIRE(player->inventory().empty());
        
        // Test that we can find objects by all their keywords
        auto room_objects = room->contents().objects;
        
        // Find bread object and test all its keywords
        std::shared_ptr<Object> bread_obj = nullptr;
        std::shared_ptr<Object> sword_obj = nullptr;
        
        for (const auto& obj : room_objects) {
            if (obj->id() == EntityId{3010}) {
                bread_obj = obj;
            } else if (obj->id() == EntityId{3020}) {
                sword_obj = obj;
            }
        }
        
        REQUIRE(bread_obj != nullptr);
        REQUIRE(sword_obj != nullptr);
        
        // Test bread object keyword matching (what take/drop commands use)
        REQUIRE(bread_obj->matches_keyword("bread"));  // take bread
        REQUIRE(bread_obj->matches_keyword("loaf"));   // take loaf  
        REQUIRE(bread_obj->matches_keyword("food"));   // take food
        REQUIRE_FALSE(bread_obj->matches_keyword("sword"));
        
        // Test sword object keyword matching (what take/drop commands use)
        REQUIRE(sword_obj->matches_keyword("sword"));   // take sword
        REQUIRE(sword_obj->matches_keyword("blade"));   // take blade
        REQUIRE(sword_obj->matches_keyword("weapon"));  // take weapon
        REQUIRE_FALSE(sword_obj->matches_keyword("bread"));
        
        // Test case insensitive matching (important for user commands)
        REQUIRE(bread_obj->matches_keyword("BREAD"));
        REQUIRE(bread_obj->matches_keyword("Loaf"));
        REQUIRE(sword_obj->matches_keyword("SWORD"));
        REQUIRE(sword_obj->matches_keyword("Weapon"));
        
        // Test that inventory operations would work
        // (Simulating what the take command does)
        auto add_result = player->inventory().add_item(bread_obj);
        REQUIRE(add_result.has_value());
        REQUIRE(player->inventory().item_count() == 1);
        
        // Test that we can find the item in inventory by keywords
        auto inventory_items = player->inventory().get_all_items();
        bool found_by_bread = false;
        bool found_by_loaf = false;
        bool found_by_food = false;
        
        for (const auto& item : inventory_items) {
            if (item->matches_keyword("bread")) found_by_bread = true;
            if (item->matches_keyword("loaf")) found_by_loaf = true;
            if (item->matches_keyword("food")) found_by_food = true;
        }
        
        REQUIRE(found_by_bread);
        REQUIRE(found_by_loaf);
        REQUIRE(found_by_food);
        
        // Test that we can remove by object (what drop command does)
        REQUIRE(player->inventory().remove_item(bread_obj));
        REQUIRE(player->inventory().empty());
    }
    
    SECTION("Object display names use short descriptions") {
        auto room_objects = room->contents().objects;
        
        for (const auto& obj : room_objects) {
            if (obj->id() == EntityId{3010}) {
                // Bread should display as "some bread"
                std::string display_name = obj->display_name(false);
                REQUIRE(display_name == "some bread");
                
                std::string display_name_with_article = obj->display_name(true);
                REQUIRE(display_name_with_article == "some bread"); // "some" already implies article
            }
            if (obj->id() == EntityId{3020}) {
                // Sword should display as "a sharp sword"
                std::string display_name = obj->display_name(false);
                REQUIRE(display_name == "a sharp sword");
                
                std::string display_name_with_article = obj->display_name(true);
                REQUIRE(display_name_with_article == "a sharp sword"); // "a" already present
            }
        }
    }
    
    SECTION("JSON round-trip preserves short descriptions and keywords") {
        // Test that objects can be serialized and deserialized correctly
        auto room_objects = room->contents().objects;
        
        for (const auto& obj : room_objects) {
            // Serialize to JSON
            nlohmann::json json = obj->to_json();
            
            // Verify JSON contains expected fields
            REQUIRE(json.contains("short_description"));
            REQUIRE(json.contains("keywords"));
            REQUIRE(json.contains("id"));
            
            if (obj->id() == EntityId{3010}) {
                REQUIRE(json["short_description"] == "some bread");
                // Keywords should be parsed from name during object creation
                std::vector<std::string> keywords = json["keywords"];
                REQUIRE(std::find(keywords.begin(), keywords.end(), "bread") != keywords.end());
                REQUIRE(std::find(keywords.begin(), keywords.end(), "loaf") != keywords.end());
                REQUIRE(std::find(keywords.begin(), keywords.end(), "food") != keywords.end());
            }
            
            // Deserialize from JSON
            auto recreated_result = Object::from_json(json);
            REQUIRE(recreated_result.has_value());
            auto recreated = std::move(recreated_result.value());
            
            // Verify recreated object matches original
            REQUIRE(recreated->id() == obj->id());
            REQUIRE(recreated->short_description() == obj->short_description());
            REQUIRE(recreated->name() == obj->name());
            
            // Verify keywords work the same
            for (const std::string& keyword : obj->keywords()) {
                REQUIRE(recreated->matches_keyword(keyword));
            }
        }
    }
}

TEST_CASE("Object Loading from JSON with keywords field", "[unit][objects][json]") {
    SECTION("Object loads correctly from JSON with keywords string") {
        // Create JSON object with modern keywords format
        nlohmann::json json = {
            {"id", "3010"},
            {"type", "FOOD"},
            {"keywords", "bread loaf food"},
            {"short_description", "some bread"},
            {"description", "A fresh loaf of bread."},
            {"weight", 1},
            {"value", 5}
        };
        
        auto result = Object::from_json(json);
        REQUIRE(result.has_value());
        auto object = std::move(result.value());
        
        // Verify object properties
        REQUIRE(object->id() == EntityId{3010});
        REQUIRE(object->name() == "bread loaf food");
        REQUIRE(object->short_description() == "some bread");
        REQUIRE(object->description() == "A fresh loaf of bread.");
        
        // Verify keywords parsed from keywords field
        REQUIRE(object->matches_keyword("bread"));
        REQUIRE(object->matches_keyword("loaf"));
        REQUIRE(object->matches_keyword("food"));
        REQUIRE_FALSE(object->matches_keyword("sword"));
        
        // Verify case insensitive matching
        REQUIRE(object->matches_keyword("BREAD"));
        REQUIRE(object->matches_keyword("Loaf"));
    }
    
    SECTION("Object loads correctly from JSON with legacy name_list") {
        // Test backward compatibility with legacy name_list format
        nlohmann::json json = {
            {"id", "3011"},
            {"type", "FOOD"},
            {"name_list", "apple fruit snack"},
            {"short_description", "a red apple"},
            {"description", "A crisp red apple."}
        };
        
        auto result = Object::from_json(json);
        REQUIRE(result.has_value());
        auto object = std::move(result.value());
        
        // Verify object properties  
        REQUIRE(object->id() == EntityId{3011});
        REQUIRE(object->name() == "apple fruit snack");
        REQUIRE(object->short_description() == "a red apple");
        
        // Verify keywords parsed from legacy name_list
        REQUIRE(object->matches_keyword("apple"));
        REQUIRE(object->matches_keyword("fruit"));
        REQUIRE(object->matches_keyword("snack"));
    }
    
    SECTION("Object loads correctly from JSON with explicit keywords array") {
        nlohmann::json json = {
            {"id", "3020"},
            {"type", "WEAPON"},
            {"name", "sharp sword"},
            {"keywords", {"sword", "blade", "weapon", "steel"}},
            {"short_description", "a gleaming sword"},
            {"description", "A finely crafted steel sword."}
        };
        
        auto result = Object::from_json(json);
        REQUIRE(result.has_value());
        auto object = std::move(result.value());
        
        // Verify keywords from explicit array
        REQUIRE(object->matches_keyword("sword"));
        REQUIRE(object->matches_keyword("blade"));
        REQUIRE(object->matches_keyword("weapon"));
        REQUIRE(object->matches_keyword("steel"));
        REQUIRE_FALSE(object->matches_keyword("bread"));
    }
}