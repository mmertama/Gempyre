cmake_minimum_required (VERSION 3.8)

set(NAME unittests)
project (${NAME})

set(CMAKE_CXX_STANDARD 17)

include_directories(
     ../../gempyrelib/include
    include
)

find_package (gempyre REQUIRED PATHS ../..)

add_executable(${PROJECT_NAME}
    tests.cpp
    gui/${NAME}.html
    )

addResource(PROJECT ${PROJECT_NAME} TARGET include/${NAME}_resource.h SOURCES gui/${NAME}.html)

target_link_libraries (${PROJECT_NAME} gempyre)

