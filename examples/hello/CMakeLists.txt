cmake_minimum_required (VERSION 3.8)

set(NAME hello)
project (${NAME})

set(CMAKE_CXX_STANDARD 17)

include_directories(
    ../../gempyrelib/include
   include
)

# This is not a an example! This is just enabler to link without install, internal to Gempyre
link_directories("$<TARGET_PROPERTY:gempyre,gempyre_libs_path>")

add_executable(${PROJECT_NAME}
   src/main.cpp
   gui/${NAME}.html
   )

add_dependencies (${PROJECT_NAME} gempyre)

addResource(PROJECT ${PROJECT_NAME} TARGET include/${NAME}.html.h SOURCES gui/${NAME}.html)

# This is not a an example! This is just enabler to link without install, internal to Gempyre
target_link_libraries (${PROJECT_NAME}  PRIVATE "$<TARGET_FILE:gempyre>" "$<TARGET_PROPERTY:gempyre,gempyre_libs>")

