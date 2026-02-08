// Test that container objects load as Container instances
#include <catch2/catch_test_macros.hpp>
#include <nlohmann/json.hpp>

#include "../../src/core/object.hpp"

using json = nlohmann::json;

TEST_CASE("Container object loads as Container instance", "[container][unit]") {

    // Create a bag object JSON similar to what would be in zone files
    // This is a self-contained test that doesn't rely on external files
    json bag_json = {
        {"id", 3032},
        {"name", "small leather bag"},
        {"short_desc", "a small leather bag"},
        {"ground_desc", "A small leather bag has been left here."},
        {"type", "Container"},
        {"weight", 1},
        {"value", 100},
        {"container_info",
         {{"capacity", 50}, {"closeable", false}, {"closed", false}, {"lockable", false}, {"locked", false}}}};

    // Create object using the Object::from_json factory (same as world loading)
    auto result = Object::from_json(bag_json);
    if (!result.has_value()) {
        FAIL("Object::from_json failed: " << result.error().message << "\nBag JSON: " << bag_json.dump(2));
    }
    REQUIRE(result.has_value());

    auto object = std::move(result.value());

    // The critical test: Can we successfully cast to Container?
    // This is exactly what the put command does with std::dynamic_pointer_cast
    std::shared_ptr<Object> shared_object(object.release());
    auto container_obj = std::dynamic_pointer_cast<Container>(shared_object);

    // This should NOT be null - container type should create Container instance
    REQUIRE(container_obj != nullptr);

    // Verify it has the correct capacity
    const auto &info = container_obj->container_info();
    REQUIRE(info.capacity == 50);

    // Verify it reports as a container
    REQUIRE(shared_object->is_container() == true);
    REQUIRE(shared_object->type_name() == "Container");
}

TEST_CASE("Regular object does not cast to Container", "[container][unit]") {

    // Create a non-container object
    json sword_json = {{"id", 1001},
                       {"name", "iron sword"},
                       {"short_desc", "an iron sword"},
                       {"ground_desc", "An iron sword lies here."},
                       {"type", "Weapon"},
                       {"weight", 5},
                       {"value", 500}};

    auto result = Object::from_json(sword_json);
    REQUIRE(result.has_value());

    std::shared_ptr<Object> shared_object(result.value().release());
    auto container_obj = std::dynamic_pointer_cast<Container>(shared_object);

    // Non-container should NOT cast to Container
    REQUIRE(container_obj == nullptr);
    REQUIRE(shared_object->is_container() == false);
}
