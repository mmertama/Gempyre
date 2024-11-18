
if(BLEEDING_EDGE)
    set(LIB_WS_VER "main")    
else()
    set(LIB_WS_VER "v4.3.3")
endif()

# this was GIT_REPOSITORY https://libwebsockets.org/repo/libwebsockets
# but the respository seems not be that responding and hence changed
# to github

FetchContent_Declare(
  libwebsockets
  GIT_REPOSITORY https://github.com/warmcat/libwebsockets.git
  GIT_TAG ${LIB_WS_VER}
  GIT_PROGRESS ${HAS_PROGRESS}
)

set(LWS_WITH_SERVER ON)

if (CMAKE_BUILD_TYPE STREQUAL "Debug" AND HAS_TEST)
    set(LWS_WITH_CLIENT ON)
else()
    set(LWS_WITH_CLIENT OFF)
endif()

set(DISABLE_WERROR ON)

if(WIN32)
  set(LWS_WITH_SYS_SMD OFF)
  set(LWS_EXT_PTHREAD_LIBRARIES "")
  set(LWS_STATIC_PIC  ON)
    set(LWS_INSTALL_LIB_DIR ${CMAKE_BINARY_DIR}/foo1)
      set(LWS_INSTALL_CMAKE_DIR ${CMAKE_BINARY_DIR}/foo2)
      set(LWS_INSTALL_BIN_DIR ${CMAKE_BINARY_DIR}/foo3)
      set(LWS_INSTALL_EXAMPLES_DIR ${CMAKE_BINARY_DIR}/foo5)
  endif()

  cmake_policy(SET CMP0077 NEW)

  set(LWS_WITHOUT_TESTAPPS ON CACHE BOOL "" FORCE)
  set(LWS_WITH_MINIMAL_EXAMPLES OFF CACHE BOOL "" FORCE)
  set(LWS_WITH_STATIC ON CACHE BOOL "" FORCE)
  set(LWS_WITH_SHARED OFF CACHE BOOL "" FORCE)
  set(LWS_WITH_SSL OFF CACHE BOOL "" FORCE)
  set(LWS_ROLE_H1 ON CACHE BOOL "" FORCE)
  set(LWS_ROLE_H2 ON CACHE BOOL "" FORCE)
  set(LWS_WITH_HTTP2 ON CACHE BOOL "" FORCE)
  set(LWS_ROLE_WS ON CACHE BOOL "" FORCE)
  set(LWS_CTEST_INTERNET_AVAILABLE OFF CACHE BOOL "" FORCE)

  
  # Windows
  set(LWS_SSL_CLIENT_USE_OS_CA_CERTS OFF)

  set(SOCKETS_LIB websockets)

  # install
  FetchContent_GetProperties(libwebsockets)
  if(NOT libwebsockets_POPULATED)
    FetchContent_Populate(libwebsockets)
    add_subdirectory(${libwebsockets_SOURCE_DIR} ${libwebsockets_BINARY_DIR})
  endif()

  include_directories(${libwebsockets_SOURCE_DIR}/include ${libwebsockets_BINARY_DIR})
  set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH};${libwebsockets_SOURCE_DIR}/cmake")

  #set(WS_LIBS websockets ${LIBWEBSOCKETS_DEP_LIBS})

set(USE_LIBWEBSOCKETS TRUE)

macro(socket_dependencies TARGET)
    target_link_directories(${TARGET} PRIVATE ${libwebsockets_BINARY_DIR}/lib)
    target_compile_definitions(${TARGET} PRIVATE USE_LIBWEBSOCKETS)
endmacro()

set(UNUSED_PARAMETERS
  "${libwebsockets_SOURCE_DIR}/lib/plat/unix/unix-misc.c"
  "${libwebsockets_SOURCE_DIR}/lib/plat/unix/unix-sockets.c"
  "${libwebsockets_SOURCE_DIR}/lib/plat/unix/unix-init.c" 
)

supress_errors(FILES ${UNUSED_PARAMETERS} FLAGS "-Wno-unused-parameter")

string(STRIP "${libwebsockets_BINARY_DIR}/lib" GEMPYRE_WS_LIB_PATH)

set(GEMPYRE_WS_SOURCES 
    src/libwebsockets/server.cpp
    src/libwebsockets/lws_server.h
    )

set(GEMPYRE_WEBSOCKET_LIBRARY_NAME "libwebsocket")   

#if(WIN32)
#    set(CONNECTION_FIND Finduva.cmake)
#    set(CONNECTION_LIB_FULL ${UV_LIB_FULL})
#elseif(RASPBERRY)
#    set(CONNECTION_FIND Finduva.cmake)
#    set(CONNECTION_LIB_FULL ${UV_LIB_FULL})
#elseif(ANDROID)
#else()
#    set(CONNECTION_FIND FinduSockets.cmake)
#    set(CONNECTION_LIB_FULL ${SOCKETS_LIB_FULL})
#endif()
