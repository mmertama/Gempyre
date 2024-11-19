cmake_minimum_required (VERSION 3.18)

project (tick)

set(CMAKE_CXX_STANDARD 17)

if(MSVC)
    if (MSVC_VERSION GREATER_EQUAL "1900")
        include(CheckCXXCompilerFlag)
        CHECK_CXX_COMPILER_FLAG("/std:c++20" _cpp_latest_flag_supported)
        if (_cpp_latest_flag_supported)
            add_compile_options("/std:c++20")
        endif()
    endif()
endif()

include_directories(
     ../../gempyrelib/include
     ${CMAKE_CURRENT_BINARY_DIR}/res
     )

# This is not a an example! This is just enabler to link without install, internal to Gempyre
link_directories("$<TARGET_PROPERTY:gempyre,gempyre_libs_path>")

add_executable(${PROJECT_NAME}
    src/main.cpp
    gui/tick.html
    )

gempyre_add_resources(PROJECT ${PROJECT_NAME} TARGET include/tick_resource.h SOURCES gui/tick.html)

# This is not a an example! This is just enabler to link without install, internal to Gempyre
target_link_libraries (${PROJECT_NAME}  PRIVATE "$<TARGET_FILE:gempyre>" "$<TARGET_PROPERTY:gempyre,gempyre_libs>")



