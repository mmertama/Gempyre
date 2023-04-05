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


macro(socket_dependencies TARGET)
target_compile_definitions(${TARGET} PRIVATE USE_WEBSOCKETPP)
endmacro()

set(XWS_SOURCES 
    src/websocketpp/server.cpp
    src/websocketpp/wspp_server.h
    )


