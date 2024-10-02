function(rostd_suite name)
  add_executable(${name} ${ARGN})
  target_include_directories(${name} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
  target_link_libraries(${name} rostd)
  add_test(${name} ${EXECUTABLE_OUTPUT_PATH}/${name})
endfunction()

rostd_suite(printx_suite printx_suite.cpp)
