cmake_minimum_required(VERSION 3.16)
project(terminal-chess LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_compile_options(-Wall -Werror -Wextra -Wpedantic)

# Header-only library
add_library(chess_lib INTERFACE)
target_include_directories(chess_lib INTERFACE ${CMAKE_SOURCE_DIR}/lib)

# Testing
include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG v1.15.2
)
FetchContent_MakeAvailable(googletest)

enable_testing()
include(GoogleTest)

add_executable(unit_tests
  test/unit/types_test.cpp
  test/unit/board_test.cpp
  test/unit/move_test.cpp
  test/unit/move_generator_test.cpp
  test/unit/game_test.cpp
  test/unit/game_status_test.cpp
  test/unit/player_test.cpp
  test/unit/human_player_test.cpp
)
target_link_libraries(unit_tests PRIVATE chess_lib GTest::gtest_main)
gtest_discover_tests(unit_tests)

add_executable(integration_tests
  test/integration/game_loop_test.cpp
)
target_link_libraries(integration_tests PRIVATE chess_lib GTest::gtest_main)
gtest_discover_tests(integration_tests)

# Main executable (requires ncurses wide)
find_package(PkgConfig)
if(PKG_CONFIG_FOUND)
  pkg_check_modules(NCURSESW ncursesw)
endif()
if(NCURSESW_FOUND)
  add_executable(chess bin/main.cpp)
  target_link_libraries(chess PRIVATE chess_lib ${NCURSESW_LIBRARIES})
  target_include_directories(chess PRIVATE ${NCURSESW_INCLUDE_DIRS})
  target_compile_options(chess PRIVATE ${NCURSESW_CFLAGS_OTHER})
else()
  message(STATUS "ncursesw not found — skipping chess executable (install libncurses-dev)")
endif()
