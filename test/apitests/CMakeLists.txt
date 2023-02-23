cmake_minimum_required (VERSION 3.18)

include(GoogleTest)

project (apitests)
set(CMAKE_CXX_STANDARD 17)
include_directories(
     ../../gempyrelib/include
    include
    ${TEST_INCLUDE_DIR}
)

# This is not a an example! This is just enabler to link without install, internal to Gempyre
link_directories("$<TARGET_PROPERTY:gempyre,gempyre_libs_path>")

add_executable(${PROJECT_NAME}
    ui_tests.cpp
    core_tests.cpp
    graphics_test.cpp
    gempyre_test.h
    gempyre_test.cpp
    gui/${PROJECT_NAME}.html
    )

gtest_discover_tests(${PROJECT_NAME})

# add HTML into resources
gempyre_add_resources(PROJECT ${PROJECT_NAME}
    TARGET include/${PROJECT_NAME}_resource.h
    SOURCES gui/${PROJECT_NAME}.html)

# copy HTML to binary for tests
add_compile_definitions(TEST_HTML="${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.html")
configure_file(gui/${PROJECT_NAME}.html ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.html COPYONLY)

# set test app name
add_compile_definitions(TEST_BINARY="${TEST_PROXY_BINARY}")


add_dependencies (${PROJECT_NAME} gempyre)
find_library(gtest PATHS ${TEST_LIBRARY_DIR})

target_link_libraries (${PROJECT_NAME}
    PRIVATE gtest)
target_link_libraries (${PROJECT_NAME}  PRIVATE "$<TARGET_FILE:gempyre>" "$<TARGET_PROPERTY:gempyre,gempyre_libs>")

