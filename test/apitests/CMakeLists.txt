cmake_minimum_required (VERSION 3.18)

include(GoogleTest)

project (apitests)
set(CMAKE_CXX_STANDARD 17)
include_directories(
     ../../gempyrelib/include
    ${TEST_INCLUDE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}/res
)


if(HAS_MOCK) 
    set(CORE_TEST mock_tests.cpp)
endif()    

# This is not a an example! This is just enabler to link without install, internal to Gempyre
link_directories("$<TARGET_PROPERTY:gempyre,gempyre_libs_path>")

add_executable(${PROJECT_NAME}
    ui_tests.cpp
    ${CORE_TEST}
    graphics_test.cpp
    gempyre_test.h
    gempyre_test.cpp
    )

if(ASAN)    
    target_compile_options(${PROJECT_NAME} PRIVATE -fsanitize=address -fsanitize=leak)
    target_link_options(${PROJECT_NAME} PRIVATE  -fsanitize=address)  
endif()     

gtest_discover_tests(${PROJECT_NAME})

# add HTML into resources
gempyre_add_resources(PROJECT ${PROJECT_NAME}
    TARGET ${CMAKE_CURRENT_BINARY_DIR}/res/${PROJECT_NAME}_resource.h
    SOURCES gui/${PROJECT_NAME}.html
    gui/spiderman1.png
    gui/spiderman2.jpg)

# in windows environment adding non-source code targets with gtest
# ends up issues. This would do    
set_source_files_properties(graphics_test.cpp PROPERTIES OBJECT_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/gui/${PROJECT_NAME}.html
    ${CMAKE_CURRENT_SOURCE_DIR}/gui/spiderman1.png
    ${CMAKE_CURRENT_SOURCE_DIR}/gui/spiderman2.jpg
    )    

# copy HTML to binary for tests
add_compile_definitions(
    TEST_HTML="${CMAKE_CURRENT_SOURCE_DIR}/gui/apitests.html"
    TEST_IMAGE="${CMAKE_CURRENT_SOURCE_DIR}/gui/spiderman1.png"
    )

#configure_file(gui/${PROJECT_NAME}.html ${CMAKE_CURRENT_SOURCE_DIR}/apitests.html COPYONLY)

# set test app name
if(HAS_MOCK)
    add_compile_definitions(TEST_BINARY="${TEST_PROXY_BINARY}")
else()
    add_compile_definitions(TEST_BINARY="")    
endif()

add_dependencies (${PROJECT_NAME} gempyre)
find_library(gtest PATHS ${TEST_LIBRARY_DIR})

target_link_libraries (${PROJECT_NAME}
    PRIVATE gtest)
target_link_libraries (${PROJECT_NAME}  PRIVATE "$<TARGET_FILE:gempyre>" "$<TARGET_PROPERTY:gempyre,gempyre_libs>")

