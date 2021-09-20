cmake_minimum_required (VERSION 3.18)

project (helloworld)

set(CMAKE_CXX_STANDARD 17)

include_directories(
     ../../gempyrelib/include
)

# This is not a an example! This is just enabler to link without install, internal to Gempyre
link_directories("$<TARGET_PROPERTY:gempyre,gempyre_libs_path>")

add_executable(helloworld main.cpp)
add_dependencies (helloworld gempyre)

TARGET_SOURCES(helloworld
    PRIVATE main.cpp gui/index.html gui/graphics/favicon.ico)


gempyre_add_resources(PROJECT helloworld TARGET html_resource.h SOURCES gui/index.html gui/graphics/favicon.ico gui/css/styles.css)

# This is not a an example! This is just enabler to link without install, internal to Gempyre
target_link_libraries (${PROJECT_NAME}  PRIVATE "$<TARGET_FILE:gempyre>" "$<TARGET_PROPERTY:gempyre,gempyre_libs>")
