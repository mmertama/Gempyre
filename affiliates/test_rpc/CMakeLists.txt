cmake_minimum_required (VERSION 3.20)

project (affiliates_test_rpc LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)

include_directories(
    ../../gempyrelib/include
    ${CMAKE_CURRENT_BINARY_DIR}/res
)

# This is not a an example! This is just enabler to link without install, internal to Gempyre
link_directories("$<TARGET_PROPERTY:gempyre,gempyre_libs_path>")

add_executable(${PROJECT_NAME}
    src/affiliates_test_rpc.cpp
    gui/affiliates_test_rpc.html
    )
   

add_dependencies(${PROJECT_NAME} gempyre)

gempyre_add_resources(PROJECT ${PROJECT_NAME}
    TARGET "${CMAKE_CURRENT_BINARY_DIR}/res/affiliates_test_rpc_resource.h"
    SOURCES "gui/affiliates_test_rpc.html")

find_package (Threads REQUIRED)

target_link_libraries (${PROJECT_NAME} PRIVATE "$<TARGET_FILE:gempyre>" "$<TARGET_PROPERTY:gempyre,gempyre_libs>")
