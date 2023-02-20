cmake_minimum_required (VERSION 3.18)

set(NAME fire)
project (${NAME})

set(CMAKE_CXX_STANDARD 17)

include_directories(
    ../../gempyrelib/include
)

# This is not a an example! This is just enabler to link without install, internal to Gempyre
link_directories("$<TARGET_PROPERTY:gempyre,gempyre_libs_path>")

add_executable(${PROJECT_NAME}
   src/main.cpp
   gui/fire.html
   gui/grunge-skull.png # this makes change deps
   )

add_dependencies (${PROJECT_NAME} gempyre)

gempyre_add_resources(PROJECT ${PROJECT_NAME}
    TARGET include/fire_resources.h 
    SOURCES gui/fire.html gui/grunge-skull.png)

# This is not a an example! This is just enabler to link without install, internal to Gempyre
target_link_libraries (${PROJECT_NAME}  PRIVATE "$<TARGET_FILE:gempyre>" "$<TARGET_PROPERTY:gempyre,gempyre_libs>")
