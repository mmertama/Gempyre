cmake_minimum_required (VERSION 3.18)

set(NAME flakes)
project (${NAME}test)

include(../../scripts/gempyre.cmake)

set(CMAKE_CXX_STANDARD 17)

include_directories(
     ../../gempyrelib/include
)

#maybe only for msvc, but shall not hurt..
add_compile_options(-D_USE_MATH_DEFINES)

# This is not a an example! This is just enabler to link without install, internal to Gempyre
link_directories("$<TARGET_PROPERTY:gempyre,gempyre_libs_path>")

add_executable(${PROJECT_NAME}
    src/main.cpp
    gui/${NAME}.html
    )

add_dependencies (${PROJECT_NAME} gempyre)

gempyre_add_resources(PROJECT ${PROJECT_NAME}
    TARGET include/${NAME}_resource.h
    SOURCES gui/${NAME}.html gui/flakes.png)

# This is not a an example! This is just enabler to link without install, internal to Gempyre
target_link_libraries (${PROJECT_NAME}  PRIVATE "$<TARGET_FILE:gempyre>" "$<TARGET_PROPERTY:gempyre,gempyre_libs>")


