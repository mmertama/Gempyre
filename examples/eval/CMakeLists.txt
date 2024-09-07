cmake_minimum_required (VERSION 3.18)

set(NAME eval)
project (${NAME}test)

set(CMAKE_CXX_STANDARD 20)

include_directories(
     ../../gempyrelib/include
)

# This is not a an example! This is just enabler to link without install, internal to Gempyre
link_directories("$<TARGET_PROPERTY:gempyre,gempyre_libs_path>" "$<TARGET_FILE_DIR:gempyre>")

add_executable(${PROJECT_NAME}
    src/main.cpp
    gui/${NAME}.html
    )

add_dependencies (${PROJECT_NAME} gempyre)
gempyre_add_resources(PROJECT ${PROJECT_NAME} TARGET include/${NAME}_resource.h SOURCES gui/${NAME}.html)

# This is not a an example! This is just enabler to link without install, internal to Gempyre
target_link_libraries (${PROJECT_NAME}  PRIVATE "$<TARGET_FILE:gempyre>" "$<TARGET_PROPERTY:gempyre,gempyre_libs>")



