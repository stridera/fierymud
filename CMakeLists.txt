cmake_minimum_required(VERSION 3.14)

project(fierymud)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_compile_options(-Werror -Wno-format-overflow -Wno-format-truncation -Wno-write-strings -Wno-unused-variable)
# add_compile_options(-Werror -Wall -Wextra) # Ideal end state

# Load Third Party libraries.
include(cmake/CPM.cmake)

CPMAddPackage("gh:fmtlib/fmt#11.1.3")
CPMAddPackage("gh:nlohmann/json@3.11.3")
# CPMAddPackage("gh:catchorg/Catch2@3.2.1")
# CPMAddPackage("gh:OlivierLDff/asio.cmake#asio-1-24-0")

include(cmake/version.cmake)

# Regardless of the CMake settings, I always want us to have debug information available.
add_compile_options(-g)

# TODO: Fix code so these don't instantly crash the mud.
# set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=leak -fsanitize=address -fsanitize=undefined ")
set(CMAKE_C_COMPILER gcc)

include_directories(include)
FILE(GLOB sources src/*.cpp)
add_executable(fierymud ${sources})

target_link_libraries(fierymud PRIVATE version crypt fmt::fmt nlohmann_json::nlohmann_json)# asio::asio)

install (TARGETS fierymud DESTINATION bin)
