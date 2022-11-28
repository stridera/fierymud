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

include_directories(include)

# Regardless of the CMake settings, I always want us to have debug information available.
add_compile_options(-g)

# TODO: Fix code so these don't instantly crash the mud.
# set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=leak -fsanitize=address -fsanitize=undefined ")
set(CMAKE_C_COMPILER gcc)

FILE(GLOB sources src/*.cpp)
add_executable(fierymud ${sources})

target_link_libraries(fierymud PRIVATE crypt fmt::fmt nlohmann_json::nlohmann_json)

install (TARGETS fierymud DESTINATION bin)