cmake_minimum_required(VERSION 2.7)
project(basic_test)

################################
# GTest
################################
ADD_SUBDIRECTORY (gtest-1.7.0)
enable_testing()
include_directories(${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR})

################################
# Unit Tests
################################
# Add test cpp file
add_executable( OoOProcessor_unittest OoOProcessor_unittest.cpp )
# Link test executable against gtest & gtest_main
target_link_libraries(OoOProcessor_unittest gtest gtest_main)
add_test( OoOProcessor_unittest OoOProcessor_unittest )
