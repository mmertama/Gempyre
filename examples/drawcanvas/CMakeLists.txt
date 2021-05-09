cmake_minimum_required (VERSION 3.14)

set(NAME drawcanvas)
project (${NAME}test)

include(../../scripts/addResource.cmake)

set(CMAKE_CXX_STANDARD 17)

include_directories(
     ../../gempyrelib/include
    include
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
addResource(PROJECT ${PROJECT_NAME} TARGET include/${NAME}_resource.h SOURCES gui/${NAME}.html)

# This is not a an example! This is just enabler to link without install, internal to Gempyre
target_link_libraries (${PROJECT_NAME}  PRIVATE "$<TARGET_FILE:gempyre>" "$<TARGET_PROPERTY:gempyre,gempyre_libs>")



