cmake_minimum_required (VERSION 3.18)

project (audio)

set(CMAKE_CXX_STANDARD 17)

include_directories(
     ../../gempyrelib/include
     ${CMAKE_CURRENT_BINARY_DIR}/res
)

# This is not a an example! This is just enabler to link without install, internal to Gempyre
link_directories("$<TARGET_PROPERTY:gempyre,gempyre_libs_path>")

add_executable(${PROJECT_NAME}
    src/main.cpp
    gui/audio.html
    gui/gempyre.ogg
    )
add_dependencies (${PROJECT_NAME} gempyre)
gempyre_add_resources(PROJECT ${PROJECT_NAME} TARGET ${CMAKE_CURRENT_BINARY_DIR}/res/audio_resource.h SOURCES gui/audio.html gui/gempyre.ogg)

# This is not a an example! This is just enabler to link without install, internal to Gempyre
target_link_libraries (${PROJECT_NAME}  PRIVATE "$<TARGET_FILE:gempyre>" "$<TARGET_PROPERTY:gempyre,gempyre_libs>")



