#include <catch2/catch_test_macros.hpp>
#include "../common/lightweight_test_harness.hpp"

#include "commands/builtin_commands.hpp"
#include "commands/command_context.hpp"
#include "core/object.hpp"
#include "world/world_manager.hpp"

TEST_CASE("Object Container System", "[integration][containers]") {
    LightweightTestHarness harness;

    SECTION("Container creation and properties") {
        // Create a Container object (not Object with ObjectType::Container)
        auto container_result = Container::create(EntityId{1001}, "chest", 10);
        REQUIRE(container_result.has_value());
        auto container = std::shared_ptr<Container>(container_result.value().release());
        
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
        auto container_result = Container::create(EntityId{1001}, "chest", 10);
        REQUIRE(container_result.has_value());
        auto container = std::shared_ptr<Container>(container_result.value().release());
        
        ContainerInfo container_info;
        container_info.capacity = 10;
        container_info.closeable = true;
        container_info.closed = false;
        container_info.lockable = false;
        container_info.locked = false;
        
        container->set_container_info(container_info);
        container->add_keyword("chest");
        container->set_short_description("a wooden chest");
        
        // Cast to Object for room contents (containers inherit from Object)
        harness.get_world().get_start_room()->contents_mutable().objects.push_back(std::static_pointer_cast<Object>(container));
        
        // Add item to player inventory
        auto item_result = Object::create(EntityId{1002}, "sword", ObjectType::Weapon);
        REQUIRE(item_result.has_value());
        auto sword = std::shared_ptr<Object>(item_result.value().release());
        sword->add_keyword("sword");
        sword->set_short_description("a steel sword");
        
        harness.get_player()->inventory().add_item(sword);
        
        // Test putting item in container
        harness.execute_command("put sword chest")
               .then_output_contains("You put");
    }

    SECTION("Open and close commands basic") {
        // Add closeable chest to room
        auto container_result = Container::create(EntityId{1001}, "chest", 10);
        REQUIRE(container_result.has_value());
        auto container = std::shared_ptr<Container>(container_result.value().release());
        
        ContainerInfo container_info;
        container_info.capacity = 10;
        container_info.closeable = true;
        container_info.closed = true;
        container_info.lockable = false;
        container_info.locked = false;
        
        container->set_container_info(container_info);
        container->add_keyword("chest");
        container->set_short_description("a wooden chest");
        
        harness.get_world().get_start_room()->contents_mutable().objects.push_back(std::static_pointer_cast<Object>(container));
        
        // Test opening closed chest
        harness.execute_command("open chest")
               .then_output_contains("You open");
               
        harness.get_player()->clear_output();
        
        // Test closing opened chest
        harness.execute_command("close chest")
               .then_output_contains("You close");
    }

    SECTION("Lock and unlock commands basic") {
        // Add lockable chest to room
        auto container_result = Container::create(EntityId{1001}, "chest", 10);
        REQUIRE(container_result.has_value());
        auto container = std::shared_ptr<Container>(container_result.value().release());
        
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
        
        harness.get_world().get_start_room()->contents_mutable().objects.push_back(std::static_pointer_cast<Object>(container));
        
        // Create and give the player the key
        auto key_result = Object::create(EntityId{2001}, "key", ObjectType::Key);
        REQUIRE(key_result.has_value());
        auto key = std::shared_ptr<Object>(key_result.value().release());
        key->add_keyword("key");
        key->set_short_description("a small brass key");
        harness.get_player()->inventory().add_item(key);
        
        // Test locking chest (now player has the key)
        harness.execute_command("lock chest")
               .then_output_contains("You lock");
               
        harness.get_player()->clear_output();
        
        // Test unlocking locked chest
        harness.execute_command("unlock chest")
               .then_output_contains("You unlock");
    }

    SECTION("Commands handle non-existent objects") {
        // Test commands with non-existent objects
        harness.execute_command("open nonexistent")
               .then_output_contains("don't see");
               
        harness.get_player()->clear_output();
        
        harness.execute_command("close nonexistent")
               .then_output_contains("don't see");
               
        harness.get_player()->clear_output();
        
        harness.execute_command("lock nonexistent")
               .then_output_contains("don't see");
               
        harness.get_player()->clear_output();
        
        harness.execute_command("unlock nonexistent")
               .then_output_contains("don't see");
    }
}