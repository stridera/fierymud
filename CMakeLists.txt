# FieryMUD Server Build System - Modern and Legacy
cmake_minimum_required(VERSION 3.20)
project(FieryMUD VERSION 3.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable testing support
enable_testing()

# CPM for package management
include(cmake/CPM.cmake)

# Dependencies
CPMAddPackage("gh:fmtlib/fmt#11.2.0")
CPMAddPackage("gh:gabime/spdlog#v1.15.3")
CPMAddPackage("gh:nlohmann/json#v3.11.3")
CPMAddPackage("gh:Neargye/magic_enum#v0.9.7")
CPMAddPackage("gh:jarro2783/cxxopts#v3.3.1")
CPMAddPackage("gh:catchorg/Catch2#v3.6.0")

# Standalone Asio for networking
CPMAddPackage(
    NAME asio
    GITHUB_REPOSITORY chriskohlhoff/asio
    GIT_TAG asio-1-30-2
    DOWNLOAD_ONLY YES
)

if(asio_ADDED)
    add_library(asio INTERFACE)
    target_include_directories(asio INTERFACE ${asio_SOURCE_DIR}/asio/include)
    target_compile_definitions(asio INTERFACE 
        ASIO_STANDALONE 
        ASIO_NO_DEPRECATED
    )
endif()

# Version generation for legacy build
find_package(Git QUIET)
if(GIT_FOUND)
    execute_process(
        COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_HASH
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
else()
    set(GIT_HASH "unknown")
endif()

set(FIERYMUD_BUILD_NUMBER 1)
string(TIMESTAMP FIERYMUD_BUILD_DATE "%Y-%m-%d %H:%M:%S")

configure_file(
    "${CMAKE_SOURCE_DIR}/src/version.hpp.in"
    "${CMAKE_BINARY_DIR}/version.hpp"
    @ONLY
)

# Get all legacy source files
file(GLOB_RECURSE LEGACY_SOURCES "src/legacy/*.cpp")
list(FILTER LEGACY_SOURCES EXCLUDE REGEX "src/legacy/modern_main.cpp")

# Get modern source files
set(SOURCES
    src/main.cpp
    src/modern/core/logging.cpp
    src/modern/core/config.cpp
    src/modern/core/entity.cpp
    src/modern/core/actor.cpp
    src/modern/core/object.cpp
    src/modern/world/room.cpp
    src/modern/world/zone.cpp
    src/modern/world/world_manager.cpp
    src/modern/world/weather.cpp
    src/modern/server/world_server.cpp
    src/modern/server/modern_mud_server.cpp
    src/modern/server/network_manager.cpp
    src/modern/server/player_connection.cpp
    src/modern/server/persistence_manager.cpp
    src/modern/server/configuration_manager.cpp
    src/modern/server/gmcp_handler.cpp
    src/modern/commands/command_system.cpp
    src/modern/commands/command_context.cpp
    src/modern/commands/command_parser.cpp
    src/modern/commands/builtin_commands.cpp
)

# Test sources (without main.cpp and networking)
set(SIMPLE_TEST_SOURCES
    src/modern/core/logging.cpp
    src/modern/core/config.cpp
    src/modern/core/entity.cpp
    src/modern/core/actor.cpp
    src/modern/core/object.cpp
    src/modern/world/room.cpp
    src/modern/world/zone.cpp
    src/modern/world/world_manager.cpp
    src/modern/world/weather.cpp
    src/modern/world/world_server.cpp
    src/modern/commands/command_system.cpp
    src/modern/commands/weather_commands.cpp
    src/modern/commands/command_context.cpp
    src/modern/commands/command_parser.cpp
    src/modern/commands/builtin_commands.cpp
    src/modern/testing/test_simple_functionality.cpp
)

# Modern FieryMUD Server (fierymud)
add_executable(fierymud ${SOURCES})

target_include_directories(fierymud PRIVATE src)

target_link_libraries(fierymud PRIVATE 
    fmt::fmt 
    spdlog::spdlog
    nlohmann_json::nlohmann_json
    magic_enum::magic_enum
    cxxopts::cxxopts
    asio
)

target_compile_definitions(fierymud PRIVATE
    SPDLOG_COMPILED_LIB
    SPDLOG_FMT_EXTERNAL
)

target_compile_options(fierymud PRIVATE
    -std=gnu++23
    -Wall
    -Wextra
    -O3
    -g
)

# Comprehensive Test Suite
set(TEST_SOURCES
    # Core system sources (needed for tests)
    src/modern/core/logging.cpp
    src/modern/core/config.cpp
    src/modern/core/entity.cpp
    src/modern/core/actor.cpp
    src/modern/core/object.cpp
    src/modern/world/room.cpp
    src/modern/world/zone.cpp
    src/modern/world/world_manager.cpp
    src/modern/world/weather.cpp
    src/modern/server/world_server.cpp
    src/modern/server/player_connection.cpp
    src/modern/server/gmcp_handler.cpp
    src/modern/commands/command_system.cpp
    src/modern/commands/command_context.cpp
    src/modern/commands/command_parser.cpp
    src/modern/commands/builtin_commands.cpp
    src/modern/core/arguments.cpp
    src/modern/core/string_utils.cpp
    tests/mock_game_session.cpp
    
    # Test files
    tests/test_arguments.cpp
    tests/test_character_creation_sessions.cpp
    
    
    
    tests/test_command_system_integration.cpp
    tests/test_core_foundation.cpp
    tests/test_world_management.cpp
    tests/test_world_integration.cpp
    tests/test_multiplayer_interaction.cpp
    tests/test_performance.cpp
    tests/test_error_handling.cpp
    tests/test_session_lifecycle.cpp
    tests/test_harness_usage.cpp
    tests/test_gmcp.cpp
    tests/test_emotes.cpp
)

# Main test executable
add_executable(tests ${TEST_SOURCES})

target_include_directories(tests PRIVATE src src/modern . ${CMAKE_SOURCE_DIR} src/modern/core src/modern/world src/modern/server src/modern/commands)

target_link_libraries(tests PRIVATE 
    fmt::fmt 
    spdlog::spdlog
    nlohmann_json::nlohmann_json
    magic_enum::magic_enum
    cxxopts::cxxopts
    asio
    Catch2::Catch2WithMain
)

target_compile_definitions(tests PRIVATE
    SPDLOG_COMPILED_LIB
    SPDLOG_FMT_EXTERNAL
)

target_compile_options(tests PRIVATE
    -std=gnu++23
    -Wall
    -Wextra
    -O3
    -g
)

# CTest integration - Register the test executable with CTest
add_test(
    NAME FieryMUD_Tests
    COMMAND tests
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

# Set test properties for better CI integration
set_tests_properties(FieryMUD_Tests PROPERTIES
    TIMEOUT 300  # 5 minute timeout
    LABELS "unit;integration;session"
)

# Add specific test categories that can be run individually
add_test(
    NAME Unit_Tests
    COMMAND tests "[unit]"
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

add_test(
    NAME Integration_Tests  
    COMMAND tests "[integration]"
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

add_test(
    NAME Session_Tests
    COMMAND tests "[session]"
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

# Set properties for individual test categories
set_tests_properties(Unit_Tests PROPERTIES
    TIMEOUT 60
    LABELS "unit"
)

set_tests_properties(Integration_Tests PROPERTIES
    TIMEOUT 120
    LABELS "integration"
)

set_tests_properties(Session_Tests PROPERTIES
    TIMEOUT 180
    LABELS "session"
)

# Legacy FieryMUD Server (fierymud_legacy)
add_executable(fierymud_legacy ${LEGACY_SOURCES})

target_include_directories(fierymud_legacy PRIVATE src src/legacy ${CMAKE_BINARY_DIR})

target_link_libraries(fierymud_legacy PRIVATE 
    fmt::fmt 
    spdlog::spdlog
    nlohmann_json::nlohmann_json
    magic_enum::magic_enum
    cxxopts::cxxopts
    crypt
)

target_compile_definitions(fierymud_legacy PRIVATE
    SPDLOG_COMPILED_LIB
    SPDLOG_FMT_EXTERNAL
    LEGACY_BUILD
)

target_compile_options(fierymud_legacy PRIVATE
    -std=gnu++23
    -Wall
    -Wextra
    -O3
    -g
)

set_target_properties(fierymud_legacy PROPERTIES EXCLUDE_FROM_ALL TRUE)

