cmake_minimum_required (VERSION 2.8)

project (mmwave_frame_unit_tests)

if(UNIX)
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Werror -std=c++0x")
endif()

##################################
# Download and install GoogleTest

include(ExternalProject)
ExternalProject_Add(gtest
  URL https://github.com/google/googletest/archive/release-1.7.0.zip
  # Comment above line, and uncomment line below to use subversion.
  # SVN_REPOSITORY http://googletest.googlecode.com/svn/trunk/
  # Uncomment line below to freeze a revision (here the one for 1.7.0)
  # SVN_REVISION -r700

  PREFIX ${CMAKE_CURRENT_BINARY_DIR}/gtest
  INSTALL_COMMAND ""

)
ExternalProject_Get_Property(gtest source_dir binary_dir)

################
# Define a test
add_executable(unit_tests test_director.cc tests/unit_tests.cc)

######################################
# Configure the test to use GoogleTest
#
# If used often, could be made a macro.

add_dependencies(unit_tests gtest pthread)
include_directories(${source_dir}/include)

target_link_libraries(unit_tests ${binary_dir}/libgtest.a)
target_link_libraries(unit_tests ${binary_dir}/libgtest_main.a)
target_link_libraries(unit_tests mmwave_com_frame)

# SSL lib may be required if we test encryption using Linux call backs
find_package ( Threads REQUIRED )
#find_package ( OpenSSL REQUIRED )
target_link_libraries(unit_tests ${CMAKE_THREAD_LIBS_INIT})

##################################
# Just make the test runnable with
#   $ make test

enable_testing()
add_test(NAME    unit_tests
         COMMAND unit_tests)
