# Update Version Variables
set(BUILD_COUNTER_PATH "${CMAKE_CURRENT_BINARY_DIR}/.build")

if(NOT EXISTS "${BUILD_COUNTER_PATH}")
	file(WRITE "${BUILD_COUNTER_PATH}" "1")
endif()

file(READ "${BUILD_COUNTER_PATH}" FIERYMUD_BUILD_NUMBER)

if(NOT FIERYMUD_BUILD_NUMBER MATCHES "^[0-9]+$")
	message(WARNING "Invalid $FIERYMUD_BUILD_NUMBER - setting to 1")
	set(FIERYMUD_BUILD_NUMBER 1)
endif()

MATH(EXPR FIERYMUD_BUILD_NUMBER "(${FIERYMUD_BUILD_NUMBER} + 1) % 65536")
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