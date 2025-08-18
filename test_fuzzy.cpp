#include "src/function_registration.hpp"
#include "src/arguments.hpp"
#include "src/structs.hpp"
#include <iostream>

void test_command(CharData *ch, Arguments args) {
    std::cout << "Test command executed!" << std::endl;
}

void clan_members_test(CharData *ch, Arguments args) {
    std::cout << "clan_members command executed!" << std::endl;
}

int main() {
    // Register some test functions
    FunctionRegistry::register_function("test_command", test_command, 0);
    FunctionRegistry::register_function("clan_members", clan_members_test, 0, CommandCategory::CLAN);
    
    // Create a dummy character for testing
    CharData dummy_ch{};
    Arguments dummy_args("");
    
    std::cout << "Testing exact match: 'clan_members'" << std::endl;
    FunctionRegistry::call_by_abbrev("clan_members", &dummy_ch, dummy_args);
    
    std::cout << "Testing prefix match: 'clan_memb'" << std::endl;
    FunctionRegistry::call_by_abbrev("clan_memb", &dummy_ch, dummy_args);
    
    std::cout << "Testing fuzzy match: 'clan_memnber'" << std::endl;
    FunctionRegistry::call_by_abbrev("clan_memnber", &dummy_ch, dummy_args);
    
    std::cout << "Testing short fuzzy match (should fail): 'abc'" << std::endl;
    FunctionRegistry::call_by_abbrev("abc", &dummy_ch, dummy_args);
    
    return 0;
}