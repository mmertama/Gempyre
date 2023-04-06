
if(BLEEDING_EDGE)
    set(LIB_WS_VER "main")    
else()
    set(LIB_WS_VER "v4.3.2")
endif()

FetchContent_Declare(
  libwebsockets
  GIT_REPOSITORY https://libwebsockets.org/repo/libwebsockets
  GIT_TAG ${LIB_WS_VER}
  GIT_PROGRESS TRUE
)

set(LWS_WITHOUT_SERVER OFF)

if (CMAKE_BUILD_TYPE STREQUAL "Debug" AND HAS_TEST)
    set(LWS_WITHOUT_CLIENT OFF)
else()
    set(LWS_WITHOUT_CLIENT ON)
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

  # install
  FetchContent_GetProperties(libwebsockets)
  if(NOT libwebsockets_POPULATED)
    FetchContent_Populate(libwebsockets)
    add_subdirectory(${libwebsockets_SOURCE_DIR} ${libwebsockets_BINARY_DIR} EXCLUDE_FROM_ALL)
  endif()

  include_directories(${libwebsockets_SOURCE_DIR}/include ${libwebsockets_BINARY_DIR})
  set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH};${libwebsockets_SOURCE_DIR}/cmake")

  set(WS_LIBS websockets ${LIBWEBSOCKETS_DEP_LIBS})

  set(USE_LIBWEBSOCKETS TRUE)

macro(socket_dependencies TARGET)
    target_compile_definitions(${TARGET} PRIVATE USE_LIBWEBSOCKETS)
endmacro()

set(GEMPYRE_WS_SOURCES 
    src/libwebsockets/server.cpp
    src/libwebsockets/lws_server.h
    )

