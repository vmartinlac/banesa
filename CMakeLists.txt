
cmake_minimum_required(VERSION 3.1)

project(BayesianNetworkSampler)

option(BUILD_EXAMPLES "Whether to build example program." OFF)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

if(BUILD_EXAMPLES)
    find_package(OpenCV REQUIRED COMPONENTS core)
endif()

find_package(TBB REQUIRED)

find_package(PkgConfig REQUIRED)
pkg_check_modules(sqlite3 IMPORTED_TARGET sqlite3)

add_subdirectory(src)
if(BUILD_EXAMPLES)
    add_subdirectory(example)
endif()

