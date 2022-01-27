cmake_minimum_required(VERSION 3.15)
project(fierymud C)
set(CMAKE_VERBOSE_MAKEFILE ON)

# gcc *.c -lm -m32 -lcrypt -ggdb3 -Wall -pedantic -fpack-struct=4 -o ../bin/test

set(CMAKE_C_COMPILER gcc)
set(CMAKE_C_FLAGS "-m32 -ggdb3 -O0 -Wall -Wextra")

FILE(GLOB sources src/*.c)
add_executable(fierymud ${sources})

target_link_libraries(fierymud m)
target_link_libraries(fierymud crypt)
# add_definitions(-DNOCRYPT)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
