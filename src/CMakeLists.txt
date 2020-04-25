
add_library(
    banesa
    SHARED
    banesa_core.h
    banesa_file_value.h
    banesa.h
    banesa_hidden_value.h
    banesa_primitive_value.h
    banesa_sampler.cpp
    banesa_sampler.h
    banesa_se3_value.h)

target_link_libraries(banesa PUBLIC PkgConfig::sqlite3 PRIVATE tbb)
target_include_directories(banesa INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)

