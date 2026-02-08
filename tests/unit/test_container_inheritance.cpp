// Test container inheritance system
#include <fstream>

#include <catch2/catch_test_macros.hpp>
#include <nlohmann/json.hpp>

#include "../../src/core/object.hpp"

using json = nlohmann::json;

TEST_CASE("Container objects are created as Container subclass from JSON", "[container][object]") {

    SECTION("Container created from legacy JSON format") {
        // Create JSON that matches the legacy bag format
        json bag_json = {
            {"id", 3032},
            {"vnum", "3032"},
            {"name", "bag"},
            {"short_description", "a large bag"},
            {"description", "A large bag made of leather."},
            {"keywords", "bag"},
            {"type", "CONTAINER"},
            {"values", {{"Capacity", "50"}, {"Flags", ""}, {"Key", "0"}, {"IsCorpse", "0"}, {"Weight Reduction", "0"}}},
            {"weight", "5"}};

        // Create object from JSON
        auto result = Object::from_json(bag_json);
        REQUIRE(result.has_value());

        auto object = std::move(result.value());

        // Should be a Container instance, not just a base Object
        auto *container = dynamic_cast<Container *>(object.get());
        REQUIRE(container != nullptr);

        // Check that it parsed the capacity correctly
        const auto &info = container->container_info();
        REQUIRE(info.capacity == 50);
        REQUIRE(info.weight_capacity == 500); // Should be 10x capacity

        // Should report as container
        REQUIRE(object->is_container() == true);
        REQUIRE(object->type_name() == "Container");
    }

    SECTION("Container methods work correctly") {
        // Create a simple container
        auto result = Container::create(EntityId{1}, "test bag", 10);
        REQUIRE(result.has_value());

        auto container = std::move(result.value());

        // Should start empty
        REQUIRE(container->is_empty() == true);
        REQUIRE(container->current_capacity() == 0);
        REQUIRE(container->contents_count() == 0);

        // Create a simple item to put in container
        auto item_result = Object::create(EntityId{2}, "test item", ObjectType::Other);
        REQUIRE(item_result.has_value());

        auto item = std::shared_ptr<Object>(item_result.value().release());
        item->set_weight(1);

        // Should be able to store the item
        REQUIRE(container->can_store_item(*item) == true);

        auto add_result = container->add_item(item);
        REQUIRE(add_result.has_value());

        // Container should no longer be empty
        REQUIRE(container->is_empty() == false);
        REQUIRE(container->contents_count() == 1);
        REQUIRE(container->current_capacity() == 1);
        REQUIRE(container->contents_weight() == 1);
    }
}
