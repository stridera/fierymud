#include <catch2/catch_test_macros.hpp>
#include "test_harness.hpp"

#include "commands/builtin_commands.hpp"
#include "commands/command_context.hpp"
#include "core/object.hpp"
#include "world/world_manager.hpp"

TEST_CASE("Object Container System", "[unit][containers]") {
    TestHarness harness;

    SECTION("Container creation and properties") {
        // Create a simple container object
        auto container_result = Object::create(EntityId{1001}, "chest", ObjectType::Container);
        REQUIRE(container_result.has_value());
        auto container = std::shared_ptr<Object>(container_result.value().release());
        
        // Configure as container
        ContainerInfo container_info;
        container_info.capacity = 10;
        container_info.closeable = true;
        container_info.closed = false;
        container_info.lockable = true;
        container_info.locked = false;
        container_info.key_id = EntityId{2001};
        
        container->set_container_info(container_info);
        container->add_keyword("chest");
        container->add_keyword("box");
        
        REQUIRE(container->is_container());
        
        const auto& info = container->container_info();
        REQUIRE(info.capacity == 10);
        REQUIRE(info.closeable == true);
        REQUIRE(info.closed == false);
        REQUIRE(info.lockable == true);
        REQUIRE(info.locked == false);
        REQUIRE(info.key_id == EntityId{2001});
    }

    SECTION("Put command basic functionality") {
        // Add chest to room
        auto container_result = Object::create(EntityId{1001}, "chest", ObjectType::Container);
        REQUIRE(container_result.has_value());
        auto container = std::shared_ptr<Object>(container_result.value().release());
        
        ContainerInfo container_info;
        container_info.capacity = 10;
        container_info.closeable = true;
        container_info.closed = false;
        container_info.lockable = false;
        container_info.locked = false;
        
        container->set_container_info(container_info);
        container->add_keyword("chest");
        container->set_short_description("a wooden chest");
        
        harness.start_room->contents_mutable().objects.push_back(container);
        
        // Add item to player inventory
        auto item_result = Object::create(EntityId{1002}, "sword", ObjectType::Weapon);
        REQUIRE(item_result.has_value());
        auto sword = std::shared_ptr<Object>(item_result.value().release());
        sword->add_keyword("sword");
        sword->set_short_description("a steel sword");
        
        harness.player->inventory().add_item(sword);
        
        // Test putting item in container
        harness.execute_command("put sword chest")
               .and_wait_for_output()
               .then_assert_output_contains("You put");
    }

    SECTION("Open and close commands basic") {
        // Add closeable chest to room
        auto container_result = Object::create(EntityId{1001}, "chest", ObjectType::Container);
        REQUIRE(container_result.has_value());
        auto container = std::shared_ptr<Object>(container_result.value().release());
        
        ContainerInfo container_info;
        container_info.capacity = 10;
        container_info.closeable = true;
        container_info.closed = true;
        container_info.lockable = false;
        container_info.locked = false;
        
        container->set_container_info(container_info);
        container->add_keyword("chest");
        container->set_short_description("a wooden chest");
        
        harness.start_room->contents_mutable().objects.push_back(container);
        
        // Test opening closed chest
        harness.execute_command("open chest")
               .and_wait_for_output()
               .then_assert_output_contains("You open");
               
        harness.clear_output();
        
        // Test closing opened chest
        harness.execute_command("close chest")
               .and_wait_for_output()
               .then_assert_output_contains("You close");
    }

    SECTION("Lock and unlock commands basic") {
        // Add lockable chest to room
        auto container_result = Object::create(EntityId{1001}, "chest", ObjectType::Container);
        REQUIRE(container_result.has_value());
        auto container = std::shared_ptr<Object>(container_result.value().release());
        
        ContainerInfo container_info;
        container_info.capacity = 10;
        container_info.closeable = true;
        container_info.closed = true;
        container_info.lockable = true;
        container_info.locked = false;
        container_info.key_id = EntityId{2001};
        
        container->set_container_info(container_info);
        container->add_keyword("chest");
        container->set_short_description("a wooden chest");
        
        harness.start_room->contents_mutable().objects.push_back(container);
        
        // Test locking chest
        harness.execute_command("lock chest")
               .and_wait_for_output()
               .then_assert_output_contains("You lock");
               
        harness.clear_output();
        
        // Test unlocking locked chest
        harness.execute_command("unlock chest")
               .and_wait_for_output()
               .then_assert_output_contains("You unlock");
    }

    SECTION("Commands handle non-existent objects") {
        // Test commands with non-existent objects
        harness.execute_command("open nonexistent")
               .and_wait_for_output()
               .then_assert_output_contains("don't see");
               
        harness.clear_output();
        
        harness.execute_command("close nonexistent")
               .and_wait_for_output()
               .then_assert_output_contains("don't see");
               
        harness.clear_output();
        
        harness.execute_command("lock nonexistent")
               .and_wait_for_output()
               .then_assert_output_contains("don't see");
               
        harness.clear_output();
        
        harness.execute_command("unlock nonexistent")
               .and_wait_for_output()
               .then_assert_output_contains("don't see");
    }
}