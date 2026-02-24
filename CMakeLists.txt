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
)
target_link_libraries(unit_tests PRIVATE chess_lib GTest::gtest_main)
gtest_discover_tests(unit_tests)
