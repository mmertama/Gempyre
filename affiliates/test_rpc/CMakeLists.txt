cmake_minimum_required (VERSION 3.18)

project (affiliates_test_rpc LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)

include_directories(
    ../../gempyrelib/include
    ../include
    ${CMAKE_CURRENT_BINARY_DIR}/res
)

# This is not a an example! This is just enabler to link without install, internal to Gempyre
link_directories("$<TARGET_PROPERTY:gempyre,gempyre_libs_path>")

add_executable(${PROJECT_NAME}
    src/${PROJECT_NAME}.cpp
    gui/${PROJECT_NAME}.html
    )
   

add_dependencies(${PROJECT_NAME} gempyre)

gempyre_add_resources(PROJECT ${PROJECT_NAME} TARGET 
${CMAKE_CURRENT_BINARY_DIR}/res/${PROJECT_NAME}_resource.h SOURCES gui/${PROJECT_NAME}.html)

find_package (Threads REQUIRED)

target_link_libraries (${PROJECT_NAME} PRIVATE "$<TARGET_FILE:gempyre>" "$<TARGET_PROPERTY:gempyre,gempyre_libs>")
