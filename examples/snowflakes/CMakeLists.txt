cmake_minimum_required (VERSION 3.25)

project (snowflakes)

include(../../scripts/gempyre.cmake)

set(CMAKE_CXX_STANDARD 17)

include_directories(
     ../../gempyrelib/include
     ${CMAKE_CURRENT_BINARY_DIR}/res
)

#maybe only for msvc, but shall not hurt..
add_compile_options(-D_USE_MATH_DEFINES)

# This is not a an example! This is just enabler to link without install, internal to Gempyre
link_directories("$<TARGET_PROPERTY:gempyre,gempyre_libs_path>")

add_executable(${PROJECT_NAME}
    src/main.cpp
    src/koch.cpp
    src/koch.h
    gui/snowflakes.html
    )

add_dependencies (${PROJECT_NAME} gempyre)

gempyre_add_resources(PROJECT ${PROJECT_NAME} TARGET ${CMAKE_CURRENT_BINARY_DIR}/res/snowflakes_resource.h SOURCES gui/snowflakes.html)

# This is not a an example! This is just enabler to link without install, internal to Gempyre
target_link_libraries (${PROJECT_NAME}  PRIVATE "$<TARGET_FILE:gempyre>" "$<TARGET_PROPERTY:gempyre,gempyre_libs>")

