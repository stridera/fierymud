#include <catch2/catch_test_macros.hpp>
#include "core/object.hpp"

TEST_CASE("Container Object Unit Tests", "[unit][containers][simple]") {
    
    SECTION("Container creation and basic properties") {
        // Create a simple container object directly
        auto container_result = Container::create(EntityId{1001}, "chest", 10);
        REQUIRE(container_result.has_value());
        auto container = std::move(container_result.value());
        
        REQUIRE(container->is_container());
        REQUIRE(container->current_capacity() == 0);
        REQUIRE(container->is_empty());
        REQUIRE(container->contents_count() == 0);
    }

    SECTION("Container item storage") {
        auto container_result = Container::create(EntityId{1001}, "chest", 10);
        REQUIRE(container_result.has_value());
        auto container = std::move(container_result.value());
        
        // Create items to store
        auto item1_result = Object::create(EntityId{1002}, "sword", ObjectType::Weapon);
        REQUIRE(item1_result.has_value());
        auto item1 = std::shared_ptr<Object>(item1_result.value().release());
        
        auto item2_result = Object::create(EntityId{1003}, "potion", ObjectType::Potion);
        REQUIRE(item2_result.has_value()); 
        auto item2 = std::shared_ptr<Object>(item2_result.value().release());
        
        // Test adding items
        auto add_result1 = container->add_item(item1);
        REQUIRE(add_result1.has_value());
        REQUIRE(container->contents_count() == 1);
        REQUIRE(!container->is_empty());
        
        auto add_result2 = container->add_item(item2);
        REQUIRE(add_result2.has_value());
        REQUIRE(container->contents_count() == 2);
        
        // Test finding items
        auto found_item = container->find_item(EntityId{1002});
        REQUIRE(found_item);
        REQUIRE(found_item->id() == EntityId{1002});
        
        // Test removing items
        auto removed_item = container->remove_item(EntityId{1002});
        REQUIRE(removed_item);
        REQUIRE(removed_item->id() == EntityId{1002});
        REQUIRE(container->contents_count() == 1);
    }

    SECTION("Container state management") {
        auto container_result = Container::create(EntityId{1001}, "chest", 10);
        REQUIRE(container_result.has_value());
        auto container = std::move(container_result.value());
        
        // Configure container properties
        ContainerInfo info;
        info.capacity = 5;
        info.closeable = true;
        info.closed = false;
        info.lockable = true;
        info.locked = false;
        info.key_id = EntityId{2001};
        
        container->set_container_info(info);
        
        // Test state retrieval
        const auto& retrieved_info = container->container_info();
        REQUIRE(retrieved_info.capacity == 5);
        REQUIRE(retrieved_info.closeable == true);
        REQUIRE(retrieved_info.closed == false);
        REQUIRE(retrieved_info.lockable == true);
        REQUIRE(retrieved_info.locked == false);
        REQUIRE(retrieved_info.key_id == EntityId{2001});
    }

    SECTION("Container capacity limits") {
        auto container_result = Container::create(EntityId{1001}, "small_box", 2);
        REQUIRE(container_result.has_value());
        auto container = std::move(container_result.value());
        
        // Set capacity info
        ContainerInfo info;
        info.capacity = 2;
        container->set_container_info(info);
        
        // Add items up to capacity
        auto item1_result = Object::create(EntityId{1002}, "item1", ObjectType::Other);
        REQUIRE(item1_result.has_value());
        auto item1 = std::shared_ptr<Object>(item1_result.value().release());
        
        auto item2_result = Object::create(EntityId{1003}, "item2", ObjectType::Other);
        REQUIRE(item2_result.has_value());
        auto item2 = std::shared_ptr<Object>(item2_result.value().release());
        
        auto item3_result = Object::create(EntityId{1004}, "item3", ObjectType::Other);
        REQUIRE(item3_result.has_value());
        auto item3 = std::shared_ptr<Object>(item3_result.value().release());
        
        // Should be able to add up to capacity
        REQUIRE(container->add_item(item1).has_value());
        REQUIRE(container->add_item(item2).has_value());
        REQUIRE(container->contents_count() == 2);
        
        // Should fail to add beyond capacity
        auto add_result = container->add_item(item3);
        REQUIRE(!add_result.has_value());
        REQUIRE(container->contents_count() == 2);
    }
}