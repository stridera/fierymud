/*
 * Simple test to reproduce the bread display issue
 */

#include "src/commands/builtin_commands.hpp"
#include "src/commands/command_system.hpp"
#include "src/core/actor.hpp"
#include "src/core/object.hpp"
#include "src/world/room.hpp"
#include "src/world/world_manager.hpp"

#include <iostream>
#include <sstream>

int main() {
    // Initialize the system
    auto &world_manager = WorldManager::instance();
    world_manager.initialize("data/world", false);

    // Load the bread object from JSON (ID 3010)
    nlohmann::json bread_json = {
        {"id", "3010"}, {"type", "FOOD"}, {"name_list", "bread loaf food"}, {"short_description", "some bread"}};

    std::cout << "Creating bread object from JSON...\n";
    auto bread_result = Object::from_json(bread_json);
    if (!bread_result.has_value()) {
        std::cout << "Failed to create bread object!\n";
        return 1;
    }

    auto bread = std::shared_ptr<Object>(bread_result.value().release());

    std::cout << "Bread object created:\n";
    std::cout << "  name(): " << bread->name() << "\n";
    std::cout << "  short_description(): " << bread->short_description() << "\n";

    // Test what cmd_look would display
    std::cout << "\nWhat cmd_look would display:\n";
    std::cout << "  " << bread->short_description() << "\n";

    return 0;
}