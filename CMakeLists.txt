# CMakeLists files in this project can

cmake_minimum_required (VERSION 2.8.11)
project (mmWaveManagementFrameProtocol)

# Recurse into the "UnitTests" and "lib" subdirectories. This does not actually
# cause another cmake executable to run. The same process will walk through
# the project's entire directory structure.
add_subdirectory (src/)



