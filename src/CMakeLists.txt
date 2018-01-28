# CMakeLists files in this project can
# refer to the root source directory of the project as ${HELLO_SOURCE_DIR} and
# to the root binary directory of the project as ${HELLO_BINARY_DIR}.
cmake_minimum_required (VERSION 2.8.11)
project (mmWaveManagementFrameProtocol)

# Recurse into the "UnitTests" and "lib" subdirectories. This does not actually
# cause another cmake executable to run. The same process will walk through
# the project's entire directory structure.
add_subdirectory (lib)

message ("C compiler used: ${CMAKE_C_COMPILER}")

#  Only include unit_tests if our platform supports gtest.
if ( "${CMAKE_C_COMPILER}" MATCHES "arm-none-eabi-gcc$" )
  message ("Not includeing unit_test as part of build, compiler not compatible")
else()
  add_subdirectory (unit_tests EXCLUDE_FROM_ALL)
endif()

