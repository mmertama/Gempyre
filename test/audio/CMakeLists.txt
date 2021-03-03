cmake_minimum_required (VERSION 3.16)

set(NAME audio)
project (${NAME}test)

set(CMAKE_CXX_STANDARD 17)

include_directories(
     ../../gempyrelib/include
    include
)

find_package (gempyre REQUIRED PATHS ../..)

add_executable(${PROJECT_NAME}
    src/main.cpp
    gui/${NAME}.html
    gui/gempyre.ogg
    include/audio_resource.h
    )

addResource(PROJECT ${PROJECT_NAME} TARGET include/${NAME}_resource.h SOURCES gui/${NAME}.html gui/gempyre.ogg)

target_link_libraries (${PROJECT_NAME} gempyre)



