cmake_minimum_required (VERSION 3.8)

set(NAME calc)
project (${NAME})

set(CMAKE_CXX_STANDARD 17)

include_directories(
     ../../telexlib/include
    include
)

find_package (telex REQUIRED PATHS ../..)

add_executable(${PROJECT_NAME}
    src/computor.h
    src/main.cpp
    gui/${NAME}.html
    )

addResource(PROJECT ${PROJECT_NAME} TARGET include/${NAME}_resource.h SOURCES gui/${NAME}.html gui/${NAME}.css gui/${NAME}.png)

add_custom_command(
        OUTPUT include/${NAME}.html.h
        COMMAND python ${CMAKE_CURRENT_SOURCE_DIR}/../../scripts/genStringResource.py gui/${NAME}.html include/${NAME}.html.h WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        DEPENDS gui/${NAME}.html
        )

target_link_libraries (${PROJECT_NAME} telex)

