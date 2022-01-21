
project(mock_browser)
set(CMAKE_CXX_STANDARD 17)

include(libwebsocket.cmake)

add_executable(${PROJECT_NAME}
    mock_browser.cpp
    ws_app.cpp
    ws.hpp
    ws.cpp)

# dependency so that JSON_DIR would be there
add_dependencies(${PROJECT_NAME} gempyre)

get_property(JSON_DIR TARGET libjson PROPERTY json_dir)

if(NOT EXISTS "${JSON_DIR}/nlohmann")
    message(WARNING "JSON dir not found: ${JSON_DIR}")
endif()

include_directories("${JSON_DIR}")

set(TEST_PROXY_BINARY "$<TARGET_FILE:${PROJECT_NAME}>" PARENT_SCOPE)

target_link_libraries(${PROJECT_NAME}
    PRIVATE ${WS_LIBS})