cmake_minimum_required (VERSION 3.25)

project (helloworld)

set(CMAKE_CXX_STANDARD 17)

include_directories(
     ../../gempyrelib/include
     ${CMAKE_CURRENT_BINARY_DIR}/res
)

# This is not a an example! This is just enabler to link without install, internal to Gempyre
link_directories("$<TARGET_PROPERTY:gempyre,gempyre_libs_path>")

add_executable(helloworld main.cpp)
add_dependencies (helloworld gempyre)

TARGET_SOURCES(helloworld
    PRIVATE main.cpp gui/index.html gui/graphics/favicon.ico)


gempyre_add_resources(PROJECT helloworld TARGET ${CMAKE_CURRENT_BINARY_DIR}/res/html_resource.h SOURCES gui/index.html gui/graphics/favicon.ico gui/css/styles.css)

# This is not a an example! This is just enabler to link without install, internal to Gempyre
target_link_libraries (${PROJECT_NAME}  PRIVATE "$<TARGET_FILE:gempyre>" "$<TARGET_PROPERTY:gempyre,gempyre_libs>")
