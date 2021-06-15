cmake_minimum_required (VERSION 3.16)

include(GoogleTest)

project (unittests)
set(CMAKE_CXX_STANDARD 17)
include_directories(
     ../../gempyrelib/include
    include
    ${TEST_INCLUDE_DIR}
)


add_executable(${PROJECT_NAME}
    unittests.cpp
    $<TARGET_OBJECTS:gempyre>
    )

gtest_discover_tests(${PROJECT_NAME})


add_dependencies (${PROJECT_NAME} gempyre)
find_library(gtest PATHS ${TEST_LIBRARY_DIR})
target_link_libraries (${PROJECT_NAME}  PRIVATE
    gtest
    "$<TARGET_PROPERTY:gempyre,gempyre_libs>")

