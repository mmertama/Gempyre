cmake_minimum_required (VERSION 3.25)

include(GoogleTest)

project (unittests)
set(CMAKE_CXX_STANDARD 17)
include_directories(
     ../../gempyrelib/include
     ../../gempyrelib/src/appui/core
     ../../gempyrelib/src/appui/server
    ${TEST_INCLUDE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}/res
)


add_executable(${PROJECT_NAME}
    unittests.cpp
    $<TARGET_OBJECTS:gempyre>
    )

gtest_discover_tests(${PROJECT_NAME})

add_dependencies (${PROJECT_NAME} gempyre)

find_library(gtest PATHS ${TEST_LIBRARY_DIR})
#get_filename_component(uva_dir $<TARGET_PROPERTY:gempyre,gempyre_libs_path> DIRECTORY)
target_link_directories(${PROJECT_NAME} PRIVATE $<TARGET_PROPERTY:gempyre,gempyre_libs_path>)
target_link_libraries (${PROJECT_NAME}
    gtest
    "$<TARGET_PROPERTY:gempyre,gempyre_libs>"
    )


