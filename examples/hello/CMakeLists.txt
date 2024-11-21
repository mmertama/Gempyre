cmake_minimum_required (VERSION 3.25)

project (hello)

set(CMAKE_CXX_STANDARD 17)

include_directories(
    ../../gempyrelib/include
    ${CMAKE_CURRENT_BINARY_DIR}/res
)

# This is not a an example! This is just enabler to link without install, internal to Gempyre
link_directories("$<TARGET_PROPERTY:gempyre,gempyre_libs_path>")

add_executable(${PROJECT_NAME}
   src/main.cpp
   gui/hello.html
   gui/favicon.ico
   )

add_dependencies (${PROJECT_NAME} gempyre)

gempyre_add_resources(PROJECT ${PROJECT_NAME} TARGET  ${CMAKE_CURRENT_BINARY_DIR}/res/hello.html.h SOURCES gui/hello.html gui/favicon.ico )

# This is not a an example! This is just enabler to link without install, internal to Gempyre
target_link_libraries (${PROJECT_NAME}  PRIVATE "$<TARGET_FILE:gempyre>" "$<TARGET_PROPERTY:gempyre,gempyre_libs>")
