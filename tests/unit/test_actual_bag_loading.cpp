// Test that actual bag from zone 30 loads as Container
#include <catch2/catch_test_macros.hpp>
#include "../../src/core/object.hpp"
#include <nlohmann/json.hpp>
#include <fstream>

using json = nlohmann::json;

TEST_CASE("Actual bag from zone 30 loads as Container instance", "[container][zone30]") {
    
    // Load the actual zone 30 file
    std::ifstream file("legacy/lib.default/world/30.json");
    REQUIRE(file.is_open());
    
    json zone_data;
    file >> zone_data;
    
    // Find the bag object (vnum 3032)
    json bag_json;
    bool found_bag = false;
    
    if (zone_data.contains("objects")) {
        for (const auto& obj : zone_data["objects"]) {
            if (obj.contains("vnum") && obj["vnum"].get<std::string>() == "3032") {
                bag_json = obj;
                found_bag = true;
                break;
            }
        }
    }
    
    REQUIRE(found_bag);
    
    // Create object using the Object::from_json factory (same as world loading)
    auto result = Object::from_json(bag_json);
    REQUIRE(result.has_value());
    
    auto object = std::move(result.value());
    
    // The critical test: Can we successfully cast to Container?
    // This is exactly what the put command does with std::dynamic_pointer_cast
    std::shared_ptr<Object> shared_object(object.release());
    auto container_obj = std::dynamic_pointer_cast<Container>(shared_object);
    
    // This should NOT be null anymore with our factory changes
    REQUIRE(container_obj != nullptr);
    
    // Verify it has the correct capacity from the legacy JSON
    const auto& info = container_obj->container_info();
    REQUIRE(info.capacity == 50); // From "Capacity": "50" in legacy format
    
    // Verify it reports as a container
    REQUIRE(shared_object->is_container() == true);
    REQUIRE(shared_object->type_name() == "Container");
}