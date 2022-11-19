cmake_minimum_required(VERSION 3.14)

project(fierymud)

include(FetchContent)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# TODO: Resolve all warnings!
# add_compile_options(-Werror -Wall -Wextra)

# Fetch and use libfmt
FetchContent_Declare(fmt
  GIT_REPOSITORY https://github.com/fmtlib/fmt.git
  GIT_TAG master
)
FetchContent_MakeAvailable(fmt)

include_directories(include)

# Regardless of the CMake settings, I always want us to have debug information available.
add_compile_options(-g)

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=leak -fsanitize=address -fsanitize=undefined ")
set(CMAKE_C_COMPILER gcc)

FILE(GLOB sources src/*.cpp)
add_executable(fierymud ${sources})

target_link_libraries(fierymud PRIVATE fmt::fmt)
