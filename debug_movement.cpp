#include "tests/common/lightweight_test_harness.hpp"
#include <iostream>

int main() {
    std::cout << "=== DEBUGGING MOVEMENT ===" << std::endl;
    
    LightweightTestHarness harness;
    
    std::cout << "Initial room: " << harness.current_room_id().value() << std::endl;
    
    // Try to move north
    harness.execute_command("north");
    
    std::cout << "After north command: " << harness.current_room_id().value() << std::endl;
    
    const auto& output = harness.get_player()->get_output();
    std::cout << "Command output (" << output.size() << " lines):" << std::endl;
    for (size_t i = 0; i < output.size(); ++i) {
        std::cout << "  [" << i << "]: '" << output[i] << "'" << std::endl;
    }
    
    return 0;
}