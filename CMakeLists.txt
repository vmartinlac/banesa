
cmake_minimum_required(VERSION 3.1)

project(BayesianNetworkSampler)

find_package(OpenCV REQUIRED COMPONENTS core)

find_package(PkgConfig REQUIRED)
pkg_check_modules(sqlite3 IMPORTED_TARGET sqlite3)

add_subdirectory(example)
add_subdirectory(src)

