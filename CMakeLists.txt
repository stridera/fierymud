cmake_minimum_required(VERSION 3.14)

project(fierymud)

include(FetchContent)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_compile_options(-Werror -Wno-format-overflow -Wno-format-truncation -Wno-write-strings -Wno-format -Wno-unused-variable)
# add_compile_options(-Werror -Wall -Wextra) // Ideal end state


# Fetch and use libfmt
FetchContent_Declare(fmt
  GIT_REPOSITORY https://github.com/fmtlib/fmt.git
  GIT_TAG master
)
FetchContent_MakeAvailable(fmt)

# JSON
FetchContent_Declare(json URL https://github.com/nlohmann/json/releases/download/v3.11.2/json.tar.xz)
FetchContent_MakeAvailable(json)


# Update Version Variables
set(BUILD_COUNTER_PATH "${CMAKE_CURRENT_BINARY_DIR}/.build")

if(NOT EXISTS "${BUILD_COUNTER_PATH}")
	file(WRITE "${BUILD_COUNTER_PATH}" "1")
endif()

file(READ "${BUILD_COUNTER_PATH}" $FIERYMUD_BUILD_NUMBER)

if(NOT $FIERYMUD_BUILD_NUMBER MATCHES "^[0-9]+$")
	message(WARNING "Invalid $FIERYMUD_BUILD_NUMBER - setting to 1")
	set($FIERYMUD_BUILD_NUMBER 1)
endif()

MATH(EXPR FIERYMUD_BUILD_NUMBER "(${$FIERYMUD_BUILD_NUMBER} + 1) % 65536")
file(WRITE "${BUILD_COUNTER_PATH}" ${FIERYMUD_BUILD_NUMBER})
string(TIMESTAMP FIERYMUD_BUILD_DATE)
message(STATUS "Build Number: ${FIERYMUD_BUILD_NUMBER}")


# Add the git hash to a variable so we can see it in the game
execute_process(
      COMMAND git log -1 --format=%h
      WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
      OUTPUT_VARIABLE GIT_HASH
      OUTPUT_STRIP_TRAILING_WHITESPACE
      )

configure_file(src/version.hpp.in generated_sources/version.hpp @ONLY)
add_library(version INTERFACE ${CMAKE_CURRENT_BINARY_DIR}/generated_sources/version.hpp)
target_include_directories(version INTERFACE ${CMAKE_CURRENT_BINARY_DIR}/generated_sources/)


include_directories(include)

# Regardless of the CMake settings, I always want us to have debug information available.
add_compile_options(-g)

# TODO: Fix code so these don't instantly crash the mud.
# set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=leak -fsanitize=address -fsanitize=undefined ")
set(CMAKE_C_COMPILER gcc)

FILE(GLOB sources src/*.cpp)
add_executable(fierymud ${sources})

target_link_libraries(fierymud PRIVATE version crypt fmt::fmt nlohmann_json::nlohmann_json)

install (TARGETS fierymud DESTINATION bin)
