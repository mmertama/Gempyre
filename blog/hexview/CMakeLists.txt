cmake_minimum_required (VERSION 3.8)

set(NAME hexview)
project (${NAME})

set(CMAKE_CXX_STANDARD 17)

include_directories(
    ../../telexlib/include
    ../../affiliates/include
   include
)

find_package (telex REQUIRED PATHS ../..)

add_executable(${PROJECT_NAME}
   src/main.cpp
   gui/${NAME}.html
   gui/${NAME}.css
   )

addResource(PROJECT ${PROJECT_NAME} TARGET include/${NAME}_resource.h SOURCES gui/${NAME}.html gui/${NAME}.css gui/text_x_hex.png)

target_link_libraries (${PROJECT_NAME} telex)

