
project(mock_browser)
set(CMAKE_CXX_STANDARD 17)

if(NOT USE_LIBWEBSOCKETS)
include(${CMAKE_SOURCE_DIR}/gempyrelib/cmake/libwebsockets.cmake)
endif()

link_directories("$<TARGET_PROPERTY:gempyre,gempyre_libs_path>")

add_executable(${PROJECT_NAME}
    mock_browser.cpp
    ws_app.cpp
    ws.hpp
    ws.cpp)

target_compile_definitions(${PROJECT_NAME}
    PRIVATE WS_LOG_LEVEL=1)

# dependency so that JSON_DIR would be there
add_dependencies(${PROJECT_NAME} gempyre)

get_property(JSON_DIR TARGET libjson PROPERTY json_dir)

if(NOT EXISTS "${JSON_DIR}/nlohmann")
    message(WARNING "JSON dir not found: ${JSON_DIR}")
endif()

include_directories("${JSON_DIR}")

#set(TEST_PROXY_BINARY "gdb -batch -ex run -ex bt --args $<TARGET_FILE:${PROJECT_NAME}>" PARENT_SCOPE)
set(TEST_PROXY_BINARY "$<TARGET_FILE:${PROJECT_NAME}>" PARENT_SCOPE)

if(RASPBERRY)
    target_link_libraries(${PROJECT_NAME}
        PRIVATE websockets
        PRIVATE -lstdc++
        )
else()
    target_link_libraries(${PROJECT_NAME}
        PRIVATE websockets
        )        
endif()

