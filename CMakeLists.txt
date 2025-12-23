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

# PostgreSQL C++ client library for database integration
CPMAddPackage(
    NAME libpqxx
    GITHUB_REPOSITORY jtv/libpqxx
    GIT_TAG 7.9.2
)

# Standalone Asio for networking with SSL support
find_package(OpenSSL REQUIRED)
find_package(PostgreSQL REQUIRED)

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
        ASIO_HAS_STD_CHRONO
    )
    target_link_libraries(asio INTERFACE OpenSSL::SSL OpenSSL::Crypto)
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

# Get source files
set(SOURCES
    src/main.cpp
    src/core/logging.cpp
    src/core/config.cpp
    src/core/entity.cpp
    src/core/actor.cpp
    src/text/text_format.cpp
    src/text/string_utils.cpp
    src/core/object.cpp
    src/core/shopkeeper.cpp
    src/core/combat.cpp
    src/core/formula_parser.cpp
    src/core/effect_system.cpp
    src/core/ability_executor.cpp
    src/core/spell_system.cpp
    src/world/room.cpp
    src/world/zone.cpp
    src/world/world_manager.cpp
    src/world/weather.cpp
    src/world/time_system.cpp
    src/server/world_server.cpp
    src/server/mud_server.cpp
    src/server/network_manager.cpp
    src/net/player_connection.cpp
    src/net/mssp_handler.cpp
    src/net/tls_context.cpp
    src/game/login_system.cpp
    src/server/persistence_manager.cpp
    src/server/configuration_manager.cpp
    src/database/database_config.cpp
    src/database/connection_pool.cpp
    src/database/config_loader.cpp
    src/database/world_queries.cpp
    src/database/player_queries.cpp
    src/database/social_queries.cpp
    src/database/db_parsing_utils.cpp
    src/database/game_data_cache.cpp
    src/commands/command_system.cpp
    src/commands/command_context.cpp
    src/text/rich_text.cpp
    src/text/terminal_capabilities.cpp
    src/commands/command_parser.cpp
    src/commands/builtin_commands.cpp
    src/commands/information_commands.cpp
    src/commands/magic_commands.cpp
    src/commands/character_commands.cpp
    src/commands/communication_commands.cpp
    src/commands/group_commands.cpp
    src/commands/movement_commands.cpp
    src/commands/position_commands.cpp
    src/commands/object_commands.cpp
    src/commands/admin_commands.cpp
    src/commands/combat_commands.cpp
    src/commands/economy_commands.cpp
    src/commands/social_commands.cpp
    src/commands/system_commands.cpp
    # src/commands/clan_commands.cpp  # Temporarily disabled due to conflicts
)

# Test sources (without main.cpp and networking)
set(SIMPLE_TEST_SOURCES
    src/core/logging.cpp
    src/core/config.cpp
    src/core/entity.cpp
    src/core/actor.cpp
    src/text/text_format.cpp
    src/core/object.cpp
    src/core/combat.cpp
    src/core/formula_parser.cpp
    src/core/effect_system.cpp
    src/core/ability_executor.cpp
    src/core/spell_system.cpp
    src/world/room.cpp
    src/world/zone.cpp
    src/world/world_manager.cpp
    src/world/weather.cpp
    src/world/world_server.cpp
    src/commands/command_system.cpp
    src/commands/weather_commands.cpp
    src/commands/command_context.cpp
    src/commands/command_parser.cpp
    src/commands/builtin_commands.cpp
    src/commands/information_commands.cpp
    src/commands/magic_commands.cpp
    src/commands/character_commands.cpp
    src/commands/communication_commands.cpp
    src/commands/group_commands.cpp
    src/commands/movement_commands.cpp
    src/commands/position_commands.cpp
    src/commands/object_commands.cpp
    src/commands/admin_commands.cpp
    src/commands/combat_commands.cpp
    src/commands/economy_commands.cpp
    src/commands/social_commands.cpp
    src/commands/system_commands.cpp
    src/testing/test_simple_functionality.cpp
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
    pqxx
    PostgreSQL::PostgreSQL
    OpenSSL::Crypto
    crypt
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

# Core system sources (needed for all tests)
set(CORE_TEST_SOURCES
    src/core/logging.cpp
    src/core/config.cpp
    src/core/entity.cpp
    src/core/actor.cpp
    src/text/text_format.cpp
    src/core/object.cpp
    src/core/shopkeeper.cpp
    src/core/combat.cpp
    src/core/formula_parser.cpp
    src/core/effect_system.cpp
    src/core/ability_executor.cpp
    src/core/spell_system.cpp
    src/world/room.cpp
    src/world/zone.cpp
    src/world/world_manager.cpp
    src/world/weather.cpp
    src/world/time_system.cpp
    src/server/world_server.cpp
    src/server/network_manager.cpp
    src/server/persistence_manager.cpp
    src/database/database_config.cpp
    src/database/connection_pool.cpp
    src/database/config_loader.cpp
    src/database/world_queries.cpp
    src/database/player_queries.cpp
    src/database/social_queries.cpp
    src/database/db_parsing_utils.cpp
    src/database/game_data_cache.cpp
    src/net/player_connection.cpp
    src/net/tls_context.cpp
    src/net/mssp_handler.cpp
    src/game/login_system.cpp
    src/commands/command_system.cpp
    src/commands/command_context.cpp
    src/text/rich_text.cpp
    src/text/terminal_capabilities.cpp
    src/commands/command_parser.cpp
    src/commands/builtin_commands.cpp
    src/commands/information_commands.cpp
    src/commands/magic_commands.cpp
    src/commands/character_commands.cpp
    src/commands/communication_commands.cpp
    src/commands/group_commands.cpp
    src/commands/movement_commands.cpp
    src/commands/position_commands.cpp
    src/commands/object_commands.cpp
    src/commands/admin_commands.cpp
    src/commands/combat_commands.cpp
    src/commands/economy_commands.cpp
    src/commands/social_commands.cpp
    src/commands/system_commands.cpp
    # src/commands/clan_commands.cpp  # Temporarily disabled due to conflicts
    src/core/arguments.cpp
    src/text/string_utils.cpp
)

# Unit test sources
file(GLOB_RECURSE UNIT_TEST_FILES "tests/unit/*.cpp")

# Integration test sources - prioritize stable tests
file(GLOB STABLE_INTEGRATION_TEST_FILES "tests/integration/*_stable.cpp")
file(GLOB_RECURSE INTEGRATION_TEST_FILES "tests/integration/*.cpp")

# Remove stable tests from the general integration list to avoid duplicates
list(REMOVE_ITEM INTEGRATION_TEST_FILES ${STABLE_INTEGRATION_TEST_FILES})

# Common test infrastructure
file(GLOB_RECURSE COMMON_TEST_FILES "tests/common/*.cpp")

# Unit test executable (no TestHarness dependencies)
add_executable(unit_tests 
    ${CORE_TEST_SOURCES}
    ${UNIT_TEST_FILES}
)

# Stable integration test executable (recommended)
add_executable(stable_tests 
    ${CORE_TEST_SOURCES}
    ${STABLE_INTEGRATION_TEST_FILES}
    ${COMMON_TEST_FILES}
)

# Legacy integration test executable (may segfault)
add_executable(integration_tests 
    ${CORE_TEST_SOURCES}
    ${INTEGRATION_TEST_FILES}
    ${COMMON_TEST_FILES}
)

# Legacy combined test executable (for backward compatibility)
add_executable(tests 
    ${CORE_TEST_SOURCES}
    ${UNIT_TEST_FILES}
    ${STABLE_INTEGRATION_TEST_FILES}
    ${COMMON_TEST_FILES}
)

# Configure unit tests executable
target_include_directories(unit_tests PRIVATE src src/ . ${CMAKE_SOURCE_DIR} src/core src/world src/server src/commands src/text)

# Configure stable tests executable
target_include_directories(stable_tests PRIVATE src src/ . ${CMAKE_SOURCE_DIR} src/core src/world src/server src/commands src/text)

target_link_libraries(unit_tests PRIVATE
    fmt::fmt
    spdlog::spdlog
    nlohmann_json::nlohmann_json
    magic_enum::magic_enum
    cxxopts::cxxopts
    asio
    pqxx
    PostgreSQL::PostgreSQL
    Catch2::Catch2WithMain
    OpenSSL::Crypto
    crypt
)

target_compile_definitions(unit_tests PRIVATE
    SPDLOG_COMPILED_LIB
    SPDLOG_FMT_EXTERNAL
)

target_compile_options(unit_tests PRIVATE
    -std=gnu++23
    -Wall
    -Wextra
    -O3
    -g
)

target_link_libraries(stable_tests PRIVATE
    fmt::fmt
    spdlog::spdlog
    nlohmann_json::nlohmann_json
    magic_enum::magic_enum
    cxxopts::cxxopts
    asio
    pqxx
    PostgreSQL::PostgreSQL
    Catch2::Catch2WithMain
    OpenSSL::Crypto
    crypt
)

target_compile_definitions(stable_tests PRIVATE
    SPDLOG_COMPILED_LIB
    SPDLOG_FMT_EXTERNAL
)

target_compile_options(stable_tests PRIVATE
    -std=gnu++23
    -Wall
    -Wextra
    -O3
    -g
)

# Configure integration tests executable
target_include_directories(integration_tests PRIVATE src src/ . ${CMAKE_SOURCE_DIR} src/core src/world src/server src/commands src/text)

target_link_libraries(integration_tests PRIVATE
    fmt::fmt
    spdlog::spdlog
    nlohmann_json::nlohmann_json
    magic_enum::magic_enum
    cxxopts::cxxopts
    asio
    pqxx
    PostgreSQL::PostgreSQL
    Catch2::Catch2WithMain
    OpenSSL::Crypto
    crypt
)

target_compile_definitions(integration_tests PRIVATE
    SPDLOG_COMPILED_LIB
    SPDLOG_FMT_EXTERNAL
)

target_compile_options(integration_tests PRIVATE
    -std=gnu++23
    -Wall
    -Wextra
    -O3
    -g
)

# Configure combined tests executable (legacy)
target_include_directories(tests PRIVATE src src/ . ${CMAKE_SOURCE_DIR} src/core src/world src/server src/commands src/text)

target_link_libraries(tests PRIVATE
    fmt::fmt
    spdlog::spdlog
    nlohmann_json::nlohmann_json
    magic_enum::magic_enum
    cxxopts::cxxopts
    asio
    pqxx
    PostgreSQL::PostgreSQL
    Catch2::Catch2WithMain
    OpenSSL::Crypto
    crypt
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

# CTest integration - Register separate test executables
add_test(
    NAME Unit_Tests
    COMMAND unit_tests
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

add_test(
    NAME Stable_Tests
    COMMAND stable_tests
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

add_test(
    NAME Integration_Tests  
    COMMAND integration_tests
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

add_test(
    NAME Combined_Tests
    COMMAND tests
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

# Set properties for test categories
set_tests_properties(Unit_Tests PROPERTIES
    TIMEOUT 60
    LABELS "unit"
)

set_tests_properties(Stable_Tests PROPERTIES
    TIMEOUT 30
    LABELS "integration;stable"
)

set_tests_properties(Integration_Tests PROPERTIES
    TIMEOUT 120
    LABELS "integration;legacy"
)

set_tests_properties(Combined_Tests PROPERTIES
    TIMEOUT 300  # 5 minute timeout
    LABELS "unit;integration;session"
)

# Additional fine-grained test runs using filters
add_test(
    NAME Unit_Tests_Filter
    COMMAND tests "[unit]"
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

add_test(
    NAME Integration_Tests_Filter  
    COMMAND tests "[integration]"
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

add_test(
    NAME Session_Tests_Filter
    COMMAND tests "[session]"
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

set_tests_properties(Unit_Tests_Filter PROPERTIES
    TIMEOUT 60
    LABELS "unit"
)

set_tests_properties(Integration_Tests_Filter PROPERTIES
    TIMEOUT 120
    LABELS "integration"
)

set_tests_properties(Session_Tests_Filter PROPERTIES
    TIMEOUT 180
    LABELS "session"
)

# Database integration test executable (uses same sources as fierymud except main.cpp)
add_executable(test_database
    src/database/test_database.cpp
    src/core/logging.cpp
    src/core/config.cpp
    src/core/entity.cpp
    src/core/actor.cpp
    src/text/text_format.cpp
    src/text/string_utils.cpp
    src/core/object.cpp
    src/core/shopkeeper.cpp
    src/core/combat.cpp
    src/core/spell_system.cpp
    src/core/ability_executor.cpp
    src/core/formula_parser.cpp
    src/core/effect_system.cpp
    src/world/room.cpp
    src/world/zone.cpp
    src/world/world_manager.cpp
    src/world/weather.cpp
    src/world/time_system.cpp
    src/server/world_server.cpp
    src/server/mud_server.cpp
    src/server/network_manager.cpp
    src/net/player_connection.cpp
    src/net/mssp_handler.cpp
    src/net/tls_context.cpp
    src/game/login_system.cpp
    src/server/persistence_manager.cpp
    src/server/configuration_manager.cpp
    src/database/database_config.cpp
    src/database/connection_pool.cpp
    src/database/config_loader.cpp
    src/database/world_queries.cpp
    src/database/player_queries.cpp
    src/database/social_queries.cpp
    src/database/db_parsing_utils.cpp
    src/database/game_data_cache.cpp
    src/commands/command_system.cpp
    src/commands/command_context.cpp
    src/text/rich_text.cpp
    src/text/terminal_capabilities.cpp
    src/commands/command_parser.cpp
    src/commands/builtin_commands.cpp
    src/commands/information_commands.cpp
    src/commands/magic_commands.cpp
    src/commands/character_commands.cpp
    src/commands/communication_commands.cpp
    src/commands/group_commands.cpp
    src/commands/movement_commands.cpp
    src/commands/position_commands.cpp
    src/commands/object_commands.cpp
    src/commands/admin_commands.cpp
    src/commands/combat_commands.cpp
    src/commands/economy_commands.cpp
    src/commands/social_commands.cpp
    src/commands/system_commands.cpp
)

target_include_directories(test_database PRIVATE src)

target_link_libraries(test_database PRIVATE
    fmt::fmt
    spdlog::spdlog
    nlohmann_json::nlohmann_json
    magic_enum::magic_enum
    asio
    pqxx
    PostgreSQL::PostgreSQL
    OpenSSL::Crypto
    crypt
)

target_compile_definitions(test_database PRIVATE
    SPDLOG_COMPILED_LIB
    SPDLOG_FMT_EXTERNAL
)

target_compile_options(test_database PRIVATE
    -std=gnu++23
    -Wall
    -Wextra
    -O3
    -g
)

# Comprehensive database test executable (same sources as test_database for consistency)
add_executable(test_database_comprehensive
    src/database/test_database_comprehensive.cpp
    src/core/logging.cpp
    src/core/config.cpp
    src/core/entity.cpp
    src/core/actor.cpp
    src/text/text_format.cpp
    src/text/string_utils.cpp
    src/core/object.cpp
    src/core/shopkeeper.cpp
    src/core/combat.cpp
    src/core/spell_system.cpp
    src/core/ability_executor.cpp
    src/core/formula_parser.cpp
    src/core/effect_system.cpp
    src/world/room.cpp
    src/world/zone.cpp
    src/world/world_manager.cpp
    src/world/weather.cpp
    src/world/time_system.cpp
    src/server/world_server.cpp
    src/server/mud_server.cpp
    src/server/network_manager.cpp
    src/net/player_connection.cpp
    src/net/mssp_handler.cpp
    src/net/tls_context.cpp
    src/game/login_system.cpp
    src/server/persistence_manager.cpp
    src/server/configuration_manager.cpp
    src/database/database_config.cpp
    src/database/connection_pool.cpp
    src/database/config_loader.cpp
    src/database/world_queries.cpp
    src/database/player_queries.cpp
    src/database/social_queries.cpp
    src/database/db_parsing_utils.cpp
    src/database/game_data_cache.cpp
    src/commands/command_system.cpp
    src/commands/command_context.cpp
    src/text/rich_text.cpp
    src/text/terminal_capabilities.cpp
    src/commands/command_parser.cpp
    src/commands/builtin_commands.cpp
    src/commands/information_commands.cpp
    src/commands/magic_commands.cpp
    src/commands/character_commands.cpp
    src/commands/communication_commands.cpp
    src/commands/group_commands.cpp
    src/commands/movement_commands.cpp
    src/commands/position_commands.cpp
    src/commands/object_commands.cpp
    src/commands/admin_commands.cpp
    src/commands/combat_commands.cpp
    src/commands/economy_commands.cpp
    src/commands/social_commands.cpp
    src/commands/system_commands.cpp
)

target_include_directories(test_database_comprehensive PRIVATE src)

target_link_libraries(test_database_comprehensive PRIVATE
    fmt::fmt
    spdlog::spdlog
    nlohmann_json::nlohmann_json
    magic_enum::magic_enum
    asio
    pqxx
    PostgreSQL::PostgreSQL
    OpenSSL::Crypto
    crypt
)

target_compile_definitions(test_database_comprehensive PRIVATE
    SPDLOG_COMPILED_LIB
    SPDLOG_FMT_EXTERNAL
)

target_compile_options(test_database_comprehensive PRIVATE
    -std=gnu++23
    -Wall
    -Wextra
    -O3
    -g
)

# Legacy FieryMUD Server (fierymud_legacy) - Currently disabled
# file(GLOB_RECURSE LEGACY_SOURCES "src/legacy/*.cpp")
# add_executable(fierymud_legacy ${LEGACY_SOURCES})

# target_include_directories(fierymud_legacy PRIVATE src/legacy ${CMAKE_BINARY_DIR})

# target_link_libraries(fierymud_legacy PRIVATE 
#     fmt::fmt 
#     spdlog::spdlog
#     nlohmann_json::nlohmann_json
#     magic_enum::magic_enum
#     cxxopts::cxxopts
#     crypt
# )

# target_compile_definitions(fierymud_legacy PRIVATE
#     SPDLOG_COMPILED_LIB
#     SPDLOG_FMT_EXTERNAL
#     LEGACY_BUILD
# )

# target_compile_options(fierymud_legacy PRIVATE
#     -std=gnu++23
#     -Wall
#     -Wextra
#     -O3
#     -g
# )

# set_target_properties(fierymud_legacy PROPERTIES EXCLUDE_FROM_ALL TRUE)
