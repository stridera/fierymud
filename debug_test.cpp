#include "tests/common/lightweight_test_harness.hpp"
#include <iostream>

int main() {
    LightweightTestHarness harness;
    
    std::cout << "=== Testing container interaction messages ===" << std::endl;
    
    // Test getting from closed container
    std::cout << "\n1. Testing 'get test_potion from test_chest' (should be closed):" << std::endl;
    harness.execute_command("get test_potion from test_chest");
    
    auto output = harness.get_player()->get_output();
    for (size_t i = 0; i < output.size(); ++i) {
        std::cout << "  [" << i << "]: '" << output[i] << "'" << std::endl;
    }
    
    // Clear output
    harness.get_player()->clear_output();
    
    // Test locking container
    std::cout << "\n2. Testing 'get test_key' (should work):" << std::endl;
    harness.execute_command("get test_key");
    
    output = harness.get_player()->get_output();
    for (size_t i = 0; i < output.size(); ++i) {
        std::cout << "  [" << i << "]: '" << output[i] << "'" << std::endl;
    }
    
    harness.get_player()->clear_output();
    
    // Test locking container
    std::cout << "\n3. Testing 'lock test_chest with test_key' (should work):" << std::endl;
    harness.execute_command("lock test_chest with test_key");
    
    output = harness.get_player()->get_output();
    for (size_t i = 0; i < output.size(); ++i) {
        std::cout << "  [" << i << "]: '" << output[i] << "'" << std::endl;
    }
    
    harness.get_player()->clear_output();
    
    // Test getting from locked container
    std::cout << "\n4. Testing 'get test_potion from test_chest' (should be locked):" << std::endl;
    harness.execute_command("get test_potion from test_chest");
    
    output = harness.get_player()->get_output();
    for (size_t i = 0; i < output.size(); ++i) {
        std::cout << "  [" << i << "]: '" << output[i] << "'" << std::endl;
    }
    
    return 0;
}
