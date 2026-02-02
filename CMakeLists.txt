# FieryMUD Server Build System
cmake_minimum_required(VERSION 3.20)
project(FieryMUD VERSION 3.0.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Option to enable sanitizers for debugging memory issues
# Default ON during development to catch memory bugs early
option(ENABLE_SANITIZERS "Enable Address and Undefined Behavior sanitizers" ON)

if(ENABLE_SANITIZERS)
    message(STATUS "Building with sanitizers enabled (ASAN + UBSAN)")
    add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer -g)
    add_link_options(-fsanitize=address,undefined)
endif()

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

# Lua 5.4 for scripting system
CPMAddPackage(
    NAME lua
    GITHUB_REPOSITORY lua/lua
    GIT_TAG v5.4.7
    DOWNLOAD_ONLY YES
)

if(lua_ADDED)
    # Build Lua as a static library from source
    set(LUA_SOURCES
        ${lua_SOURCE_DIR}/lapi.c
        ${lua_SOURCE_DIR}/lauxlib.c
        ${lua_SOURCE_DIR}/lbaselib.c
        ${lua_SOURCE_DIR}/lcode.c
        ${lua_SOURCE_DIR}/lcorolib.c
        ${lua_SOURCE_DIR}/lctype.c
        ${lua_SOURCE_DIR}/ldblib.c
        ${lua_SOURCE_DIR}/ldebug.c
        ${lua_SOURCE_DIR}/ldo.c
        ${lua_SOURCE_DIR}/ldump.c
        ${lua_SOURCE_DIR}/lfunc.c
        ${lua_SOURCE_DIR}/lgc.c
        ${lua_SOURCE_DIR}/linit.c
        ${lua_SOURCE_DIR}/liolib.c
        ${lua_SOURCE_DIR}/llex.c
        ${lua_SOURCE_DIR}/lmathlib.c
        ${lua_SOURCE_DIR}/lmem.c
        ${lua_SOURCE_DIR}/loadlib.c
        ${lua_SOURCE_DIR}/lobject.c
        ${lua_SOURCE_DIR}/lopcodes.c
        ${lua_SOURCE_DIR}/loslib.c
        ${lua_SOURCE_DIR}/lparser.c
        ${lua_SOURCE_DIR}/lstate.c
        ${lua_SOURCE_DIR}/lstring.c
        ${lua_SOURCE_DIR}/lstrlib.c
        ${lua_SOURCE_DIR}/ltable.c
        ${lua_SOURCE_DIR}/ltablib.c
        ${lua_SOURCE_DIR}/ltm.c
        ${lua_SOURCE_DIR}/lundump.c
        ${lua_SOURCE_DIR}/lutf8lib.c
        ${lua_SOURCE_DIR}/lvm.c
        ${lua_SOURCE_DIR}/lzio.c
    )
    add_library(lua STATIC ${LUA_SOURCES})
    target_include_directories(lua PUBLIC ${lua_SOURCE_DIR})
    target_compile_definitions(lua PUBLIC LUA_USE_POSIX LUA_USE_DLOPEN)
    set_target_properties(lua PROPERTIES C_STANDARD 11)
endif()

# Sol3 - Modern C++ Lua bindings (header-only)
CPMAddPackage(
    NAME sol2
    GITHUB_REPOSITORY ThePhD/sol2
    GIT_TAG v3.5.0
)

# PostgreSQL C++ client library for database integration
CPMAddPackage(
    NAME libpqxx
    GITHUB_REPOSITORY jtv/libpqxx
    GIT_TAG 7.9.2
)

# Redis C client library for event publishing (Muditor bridge)
find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
    pkg_check_modules(HIREDIS hiredis)
endif()

if(NOT HIREDIS_FOUND)
    # Fall back to CPM if system hiredis not found
    CPMAddPackage(
        NAME hiredis
        GITHUB_REPOSITORY redis/hiredis
        GIT_TAG v1.2.0
        OPTIONS
            "DISABLE_TESTS ON"
            "ENABLE_SSL OFF"
    )
    if(hiredis_ADDED)
        set(HIREDIS_LIBRARIES hiredis)
        set(HIREDIS_INCLUDE_DIRS ${hiredis_SOURCE_DIR})
    endif()
endif()

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

# Version generation
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

# =============================================================================
# Core Library (shared by fierymud and all test executables)
# =============================================================================
# This eliminates duplicate compilation - sources are compiled once into a
# static library that all targets link against.

set(CORE_SOURCES
    src/core/logging.cpp
    src/core/log_subscriber.cpp
    src/core/config.cpp
    src/core/entity.cpp
    src/core/actor.cpp
    src/core/money.cpp
    src/core/object.cpp
    src/core/shopkeeper.cpp
    src/core/board.cpp
    src/core/combat.cpp
    src/core/formula_parser.cpp
    src/core/effect_system.cpp
    src/core/ability_executor.cpp
    src/core/spell_system.cpp
    src/core/skill_system.cpp
    src/core/class_config.cpp
    src/core/arguments.cpp
    src/text/text_format.cpp
    src/text/string_utils.cpp
    src/text/message_utils.cpp
    src/text/rich_text.cpp
    src/text/terminal_capabilities.cpp
    src/world/room.cpp
    src/world/zone.cpp
    src/world/world_manager.cpp
    src/world/templates.cpp
    src/world/weather.cpp
    src/world/time_system.cpp
    src/server/world_server.cpp
    src/server/mud_server.cpp
    src/server/network_manager.cpp
    src/server/persistence_manager.cpp
    src/server/configuration_manager.cpp
    src/net/player_connection.cpp
    src/net/mssp_handler.cpp
    src/net/tls_context.cpp
    src/game/login_system.cpp
    src/game/composer_system.cpp
    src/database/database_config.cpp
    src/database/connection_pool.cpp
    src/database/config_loader.cpp
    src/database/world_queries.cpp
    src/database/player_queries.cpp
    src/database/player_repository.cpp
    src/database/social_queries.cpp
    src/database/db_parsing_utils.cpp
    src/database/game_data_cache.cpp
    src/database/quest_queries.cpp
    src/database/trigger_queries.cpp
    src/commands/command_system.cpp
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
    src/commands/account_commands.cpp
    src/commands/admin_commands.cpp
    src/commands/combat_commands.cpp
    src/commands/skill_commands.cpp
    src/commands/economy_commands.cpp
    src/commands/postmaster_commands.cpp
    src/commands/social_commands.cpp
    src/commands/system_commands.cpp
    src/commands/quest_commands.cpp
    # Scripting system
    src/scripting/script_engine.cpp
    src/scripting/trigger_manager.cpp
    src/scripting/coroutine_scheduler.cpp
    src/scripting/script_timer_manager.cpp
    src/scripting/bindings/lua_actor.cpp
    src/scripting/bindings/lua_room.cpp
    src/scripting/bindings/lua_object.cpp
    src/scripting/bindings/lua_quest.cpp
    src/scripting/bindings/lua_combat.cpp
    src/scripting/bindings/lua_world.cpp
    src/scripting/bindings/lua_spells.cpp
    src/scripting/bindings/lua_zone.cpp
    src/scripting/bindings/lua_exit.cpp
    src/scripting/bindings/lua_skills.cpp
    src/scripting/bindings/lua_effects.cpp
    src/scripting/bindings/lua_timers.cpp
    src/scripting/bindings/lua_vars.cpp
    src/scripting/bindings/lua_templates.cpp
    # Quest system
    src/quests/quest_manager.cpp
    # Event publishing for Muditor bridge
    src/events/event_publisher.cpp
)

# Create the core static library
add_library(fierymud_core STATIC ${CORE_SOURCES})

target_include_directories(fierymud_core PUBLIC
    src
    ${HIREDIS_INCLUDE_DIRS}
)

target_link_libraries(fierymud_core PUBLIC
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
    lua
    ${CMAKE_DL_LIBS}
    sol2::sol2
    ${HIREDIS_LIBRARIES}
)

target_compile_definitions(fierymud_core PUBLIC
    SPDLOG_COMPILED_LIB
    SPDLOG_FMT_EXTERNAL
)

target_compile_options(fierymud_core PRIVATE
    -std=gnu++23
    -Wall
    -Wextra
    -O3
    -g
)

# =============================================================================
# Modern FieryMUD Server (fierymud)
# =============================================================================
# Only compiles main.cpp and admin server - links to fierymud_core for everything else

add_executable(fierymud
    src/main.cpp
    src/admin/admin_server.cpp
    src/admin/zone_reload_handler.cpp
    src/admin/player_handler.cpp
)

target_link_libraries(fierymud PRIVATE fierymud_core)

target_compile_options(fierymud PRIVATE
    -std=gnu++23
    -Wall
    -Wextra
    -O3
    -g
)

# =============================================================================
# Test Executables (link to fierymud_core - no duplicate compilation)
# =============================================================================

# Unit test sources
file(GLOB_RECURSE UNIT_TEST_FILES "tests/unit/*.cpp")

# Integration test sources (stable tests only)
file(GLOB STABLE_INTEGRATION_TEST_FILES "tests/integration/*_stable.cpp")

# Common test infrastructure
file(GLOB_RECURSE COMMON_TEST_FILES "tests/common/*.cpp")

# Unit test executable
add_executable(unit_tests ${UNIT_TEST_FILES})
target_link_libraries(unit_tests PRIVATE fierymud_core Catch2::Catch2WithMain)
target_compile_options(unit_tests PRIVATE -std=gnu++23 -Wall -Wextra -O3 -g)

# Stable integration test executable (recommended)
add_executable(stable_tests ${STABLE_INTEGRATION_TEST_FILES} ${COMMON_TEST_FILES})
target_link_libraries(stable_tests PRIVATE fierymud_core Catch2::Catch2WithMain)
target_compile_options(stable_tests PRIVATE -std=gnu++23 -Wall -Wextra -O3 -g)

# Combined test executable
add_executable(tests ${UNIT_TEST_FILES} ${STABLE_INTEGRATION_TEST_FILES} ${COMMON_TEST_FILES})
target_link_libraries(tests PRIVATE fierymud_core Catch2::Catch2WithMain)
target_compile_options(tests PRIVATE -std=gnu++23 -Wall -Wextra -O3 -g)

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

# =============================================================================
# Database Test Executables (link to fierymud_core)
# =============================================================================

# Database integration test executable
add_executable(test_database src/database/test_database.cpp)
target_link_libraries(test_database PRIVATE fierymud_core)
target_compile_options(test_database PRIVATE -std=gnu++23 -Wall -Wextra -O3 -g)

# Comprehensive database test executable
add_executable(test_database_comprehensive src/database/test_database_comprehensive.cpp)
target_link_libraries(test_database_comprehensive PRIVATE fierymud_core)
target_compile_options(test_database_comprehensive PRIVATE -std=gnu++23 -Wall -Wextra -O3 -g)
