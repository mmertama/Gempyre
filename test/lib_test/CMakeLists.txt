cmake_minimum_required (VERSION 3.18)

project(lib_test LANGUAGES CXX)

find_package(gempyre REQUIRED)

include(gempyre)

set(CMAKE_CXX_STANDARD 17)
   
add_executable(${PROJECT_NAME}
      main.cpp
)

include_directories(
      ${CMAKE_CURRENT_BINARY_DIR}/res
   )


gempyre_add_resources(PROJECT ${PROJECT_NAME} 
      TARGET ${CMAKE_CURRENT_BINARY_DIR}/res/test.h
      SOURCES favicon.ico)

target_link_libraries (${PROJECT_NAME}  gempyre::gempyre)   

 
