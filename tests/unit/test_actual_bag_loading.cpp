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
    
    if (zone_data.contains("OBJECT")) {
        std::vector<std::string> found_vnums;
        for (const auto& obj : zone_data["OBJECT"]) {
            if (obj.contains("vnum")) {
                // Try both string and numeric vnum formats
                std::string vnum_str;
                if (obj["vnum"].is_string()) {
                    vnum_str = obj["vnum"].get<std::string>();
                } else if (obj["vnum"].is_number()) {
                    vnum_str = std::to_string(obj["vnum"].get<int>());
                }
                
                found_vnums.push_back(vnum_str);
                
                if (vnum_str == "3032") {
                    bag_json = obj;
                    // Add the missing id field that Entity::from_json requires
                    if (!bag_json.contains("id")) {
                        bag_json["id"] = 3032;
                    }
                    // Add a name if missing (Entity requires non-empty name)
                    if (!bag_json.contains("name") || bag_json["name"].get<std::string>().empty()) {
                        // Use name_list or create a default
                        if (bag_json.contains("name_list") && !bag_json["name_list"].get<std::string>().empty()) {
                            bag_json["name"] = bag_json["name_list"];
                        } else {
                            bag_json["name"] = "bag"; // Default name
                        }
                    }
                    found_bag = true;
                    break;
                }
            }
        }
        if (!found_bag) {
            // Debug output to see what vnums are available
            std::string vnum_list;
            for (const auto& v : found_vnums) {
                if (!vnum_list.empty()) vnum_list += ", ";
                vnum_list += v;
            }
            FAIL("Did not find vnum 3032. Found vnums: " << vnum_list);
        }
    } else {
        FAIL("Zone data does not contain OBJECT key");
    }
    
    REQUIRE(found_bag);
    
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
    
    // This should NOT be null anymore with our factory changes
    REQUIRE(container_obj != nullptr);
    
    // Verify it has the correct capacity from the legacy JSON
    const auto& info = container_obj->container_info();
    REQUIRE(info.capacity == 50); // From "Capacity": "50" in legacy format
    
    // Verify it reports as a container
    REQUIRE(shared_object->is_container() == true);
    REQUIRE(shared_object->type_name() == "Container");
}