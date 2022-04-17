cmake_minimum_required (VERSION 3.18)

project(Hello LANGUAGES CXX)

find_package (gempyre REQUIRED)

include(gempyre)

set(CMAKE_CXX_STANDARD 17)

include_directories(
   include
   ${CMAKE_CURRENT_BINARY_DIR}/include # resource is added here
)

add_executable(${PROJECT_NAME}
   src/main.cpp
   gui/hello.html
   )

gempyre_add_resources(PROJECT ${PROJECT_NAME} TARGET ${CMAKE_CURRENT_BINARY_DIR}/include/hello.html.h SOURCES gui/hello.html)

target_link_libraries (${PROJECT_NAME}  gempyre::gempyre)
