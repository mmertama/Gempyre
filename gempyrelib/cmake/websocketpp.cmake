message(FATAL_ERROR "Websocket++ is TODO")

if(BLEEDING_EDGE)
    set(LIB_WS_VER master)    
else()
    set(LIB_WS_VER 0.8.2)
endif()

externalproject_add(libwebsockets
    GIT_REPOSITORY https://github.com/zaphoyd/websocketpp.git
    GIT_TAG ${LIB_WS_VER}
    GIT_PROGRESS true
    UPDATE_DISCONNECTED false
    BUILD_COMMAND ""
    CONFIGURE_COMMAND ""
    INSTALL_COMMAND ""
    UPDATE_COMMAND ""
    PATCH_COMMAND ""
    TEST_COMMAND ""
    )

# set include path there    
ExternalProject_Get_Property(libwebsockets SOURCE_DIR)
set(WEBSOCKETS_SOURCES ${SOURCE_DIR})
set(SYSTEM_INCLUDES "${SYSTEM_INCLUDES} ${SOURCE_DIR}")    


macro(socket_dependencies TARGET)
target_compile_definitions(${TARGET} PRIVATE USE_WEBSOCKETPP)
endmacro()

set(GEMPYRE_WS_SOURCES 
    src/websocketpp/server.cpp
    src/websocketpp/wspp_server.h
    )


set(GEMPYRE_WEBSOCKET_LIBRARY_NAME "websocketcpp") 

