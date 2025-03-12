cmake_minimum_required(VERSION 3.14)

project(fierymud)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_compile_options(-Werror -Wno-format-overflow -Wno-format-truncation -Wno-write-strings -Wno-unused-variable)
# add_compile_options(-Werror -Wall -Wextra) # Ideal end state

# Load Third Party libraries.
include(cmake/CPM.cmake)

CPMAddPackage("gh:fmtlib/fmt#11.1.3")
CPMAddPackage("gh:Neargye/magic_enum@0.9.7")
CPMAddPackage("gh:nlohmann/json@3.11.3")
cpmaddpackage("gh:jarro2783/cxxopts#v3.1.1")

# Testing
CPMAddPackage("gh:catchorg/Catch2@3.8.0")

# Networking... eventually.
# CPMAddPackage("gh:OlivierLDff/asio.cmake#asio-1-24-0")

include(cmake/version.cmake)

# Regardless of the CMake settings, I always want us to have debug information available.
add_compile_options(-g)

include_directories(src/)
FILE(GLOB sources src/*.cpp)


# Testing using Catch2
FILE(GLOB test_files test/*.cpp)
add_executable(tests ${sources} ${test_files})

target_link_libraries(tests PRIVATE Catch2::Catch2  version crypt fmt::fmt nlohmann_json::nlohmann_json magic_enum::magic_enum)

list(APPEND CMAKE_MODULE_PATH ${Catch2_SOURCE_DIR}/extras)

# Echo the CMake module path to the console.
message(STATUS "CMake Module Path: ${CMAKE_MODULE_PATH}")


include(CTest)
include(Catch)
catch_discover_tests(tests)

# TODO: Fix code so these don't instantly crash the mud.
# set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=leak -fsanitize=address -fsanitize=undefined ")
set(CMAKE_C_COMPILER gcc)


add_executable(fierymud ${sources})

target_link_libraries(fierymud PRIVATE version crypt fmt::fmt nlohmann_json::nlohmann_json magic_enum::magic_enum)# asio::asio)

install (TARGETS fierymud DESTINATION bin)
