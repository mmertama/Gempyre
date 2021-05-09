cmake_minimum_required (VERSION 3.8)

project (helloworld)

set(CMAKE_CXX_STANDARD 17)

include_directories(
     ../../gempyrelib/include
)

# This is not a an example! This is just enabler to link without install, internal to Gempyre
link_directories("$<TARGET_PROPERTY:gempyre,gempyre_libs_path>")

add_executable(helloworld main.cpp)

add_dependencies (helloworld gempyre)

# This is not a an example! This is just enabler to link without install, internal to Gempyre
target_link_libraries (helloworld  PRIVATE "$<TARGET_FILE:gempyre>" "$<TARGET_PROPERTY:gempyre,gempyre_libs>")



