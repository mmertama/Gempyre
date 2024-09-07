cmake_minimum_required (VERSION 3.18)
set(NAME affiliates_test_rpc)
project (${NAME})

set(CMAKE_CXX_STANDARD 20)

include_directories(
    ../../gempyrelib/include
    ../include
    include
)

# This is not a an example! This is just enabler to link without install, internal to Gempyre
link_directories("$<TARGET_PROPERTY:gempyre,gempyre_libs_path>")

add_executable(${PROJECT_NAME}
    src/${NAME}.cpp
    gui/${NAME}.html
    )
   

add_dependencies(${PROJECT_NAME} gempyre)

gempyre_add_resources(PROJECT ${PROJECT_NAME} TARGET include/${NAME}_resource.h SOURCES gui/${NAME}.html)

find_package (Threads REQUIRED)

target_link_libraries (${PROJECT_NAME}  PRIVATE "$<TARGET_FILE:gempyre>" "$<TARGET_PROPERTY:gempyre,gempyre_libs>")
