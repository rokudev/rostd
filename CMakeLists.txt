# rostd maintainers

cmake_minimum_required(VERSION 3.8)
project(rostd CXX)

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/lib)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)

add_subdirectory(cmake)

if (MSVC)
  target_compile_options(rostd INTERFACE /W3)
else()
  target_compile_options(rostd INTERFACE -std=c++20 -Wall -Wextra -pedantic -Werror=return-type)
endif()

enable_testing()
add_subdirectory(test)
