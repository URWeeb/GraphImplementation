cmake_minimum_required(VERSION 3.14)
project(graph_algorithms CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

add_compile_options(-Wall -Wextra)
add_executable(graph_cli src/graph_algos.cpp)
option(BUILD_TESTS "Build unit tests" ON)

if(BUILD_TESTS)
  include(FetchContent)
  FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.15.2
  )
  set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
  FetchContent_MakeAvailable(googletest)

  enable_testing()

  add_executable(graph_tests test/graph_test.cpp)
  target_link_libraries(graph_tests PRIVATE gtest gtest_main)

  include(GoogleTest)
  gtest_discover_tests(graph_tests)
endif()