cmake_minimum_required (VERSION 3.25)

set(NAME imageblit)
project (imageblit)

include(../../scripts/gempyre.cmake)

set(CMAKE_CXX_STANDARD 17)

include_directories(
     ../../gempyrelib/include
     ${CMAKE_CURRENT_BINARY_DIR}/res
)

# This is not a an example! This is just enabler to link without install, internal to Gempyre
link_directories("$<TARGET_PROPERTY:gempyre,gempyre_libs_path>")

set(STUFF
    free-scenery-7.jpg
    hiclipart.com.png
    hiclipart.png
    tom-hanssens-shot-01.jpg
)

set(RES
    stuff/owl.png
    stuff/alpha.png
    stuff/captainamerica.jpg
)

add_executable(${PROJECT_NAME}
    src/main.cpp
    gui/imageblit.html
    ${RES}
    ${STUFF}
    )
add_dependencies (${PROJECT_NAME} gempyre)

gempyre_add_resources(PROJECT ${PROJECT_NAME} TARGET ${CMAKE_CURRENT_BINARY_DIR}/res/imageblit_resource.h
SOURCES gui/imageblit.html ${RES} )


foreach(IMG ${STUFF})
    file(COPY  ${CMAKE_CURRENT_SOURCE_DIR}/stuff/${IMG}
        DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
endforeach()

target_compile_definitions(${PROJECT_NAME} PRIVATE IMAGE_FOLDER=${CMAKE_CURRENT_BINARY_DIR})

# This is not a an example! This is just enabler to link without install, internal to Gempyre
target_link_libraries (${PROJECT_NAME}  PRIVATE "$<TARGET_FILE:gempyre>" "$<TARGET_PROPERTY:gempyre,gempyre_libs>")



