cmake_minimum_required (VERSION 3.25)

project (minihello)

set(CMAKE_CXX_STANDARD 17)

include_directories(
    ../../gempyrelib/include
)

# This is not a an example! This is just enabler to link without install, internal to Gempyre
link_directories("$<TARGET_PROPERTY:gempyre,gempyre_libs_path>")

add_executable(${PROJECT_NAME}
    src/main.cpp
    gui/minihello.html
    )
add_dependencies (${PROJECT_NAME} gempyre)

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/gui/minihello.html
    DESTINATION ${CMAKE_CURRENT_BINARY_DIR}
    )
# This is not a an example! This is just enabler to link without install, internal to Gempyre
target_link_libraries (${PROJECT_NAME}  PRIVATE "$<TARGET_FILE:gempyre>" "$<TARGET_PROPERTY:gempyre,gempyre_libs>")


