if(BLEEDING_EDGE)
    set(LIB_WS_VER master)    
else()
    set(LIB_WS_VER 0.8.2)
endif()

macro(socket_dependencies TARGET)
target_compile_definitions(${TARGET} PRIVATE USE_WEBSOCKETPP)
endmacro()


