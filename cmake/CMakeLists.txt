# rostd integration

cmake_minimum_required(VERSION 3.8)
project(rostd CXX)

get_filename_component(ROSTD_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../" ABSOLUTE)
if (NOT DEFINED ROSTD_BUILD_DIR)
  set(ROSTD_BUILD_DIR ${CMAKE_CURRENT_BINARY_DIR})
endif()
set(LIBRARY_OUTPUT_PATH ${ROSTD_BUILD_DIR}/lib)
set(EXECUTABLE_OUTPUT_PATH ${ROSTD_BUILD_DIR}/bin)

# Require at least C++11
set(ROSTD_FEATURES cxx_std_11)

###############################################################################
# rostd package
###############################################################################

find_package(Threads REQUIRED)

add_library(rostd INTERFACE)

set_target_properties(rostd PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES "${ROSTD_ROOT}/include"
  INTERFACE_COMPILE_FEATURES "${ROSTD_FEATURES}"
  INTERFACE_LINK_LIBRARIES "${CMAKE_THREAD_LIBS_INIT}")

if (MSVC)
  # https://devblogs.microsoft.com/cppblog/msvc-now-correctly-reports-__cplusplus/
  target_compile_options(rostd INTERFACE /Zc:__cplusplus)
endif()
