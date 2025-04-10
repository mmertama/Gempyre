
if(BLEEDING_EDGE)
    set(LIB_WS_VER "main")    
else()
    set(LIB_WS_VER "v4.3.3")
endif()

set(CMAKE_SUPPRESS_DEVELOPER_WARNINGS 1 CACHE INTERNAL "Suppress developer warnings")

# this was GIT_REPOSITORY https://libwebsockets.org/repo/libwebsockets
# but the respository seems not be that responding and hence changed
# to github

FetchContent_Declare(
  libwebsockets
  GIT_REPOSITORY https://github.com/warmcat/libwebsockets.git
  GIT_TAG ${LIB_WS_VER}
  GIT_PROGRESS ${HAS_PROGRESS}
  CMAKE_ARGS -DCMAKE_POLICY_VERSION_MINIMUM=3.5 
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
endif()
  # to not pollute build itself
  set(LWS_INSTALL_LIB_DIR ${CMAKE_BINARY_DIR}/foo1)
  set(LWS_INSTALL_CMAKE_DIR ${CMAKE_BINARY_DIR}/foo2)
  set(LWS_INSTALL_BIN_DIR ${CMAKE_BINARY_DIR}/foo3)
  #set(LWS_INSTALL_INCLUDE_DIR ${CMAKE_BINARY_DIR}/foo4)
  set(LWS_INSTALL_EXAMPLES_DIR ${CMAKE_BINARY_DIR}/foo5)
#else()
#  set(LWS_INSTALL_LIB_DIR /dev/null)
#  set(LWS_INSTALL_CMAKE_DIR /dev/null)
#  set(LWS_INSTALL_BIN_DIR /dev/null)
  #set(LWS_INSTALL_INCLUDE_DIR ${CMAKE_BINARY_DIR}/foo4)
#  set(LWS_INSTALL_EXAMPLES_DIR /dev/null)
#endif()

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

#target_compile_options(libwebsockets PRIVATE -Wno-unused-parameter)

if (MSVC)
  target_compile_options(websockets PRIVATE /wd5102 /wd4005)
else()
  target_compile_options(websockets PRIVATE -Wno-unused-parameter)
  target_compile_options(websockets PRIVATE -Wno-shadow)

endif()

if (MINGW)
  target_compile_options(websockets PRIVATE
  -Wno-redundant-decls
  -Wno-builtin-macro-redefined
  -Wno-macro-redefined
  )
#add_compile_definitions(websockets PRIVATE _WIN32_WINNT=0x0601) # must be for libwebsocket   
endif()


set(USE_LIBWEBSOCKETS TRUE)

target_compile_definitions(websockets PRIVATE SUPPRESS_WS_ERRORS)

macro(socket_dependencies TARGET)
    target_link_directories(${TARGET} PRIVATE ${libwebsockets_BINARY_DIR}/lib)
    target_compile_definitions(${TARGET} PRIVATE USE_LIBWEBSOCKETS)
    add_dependencies(${TARGET} websockets)
endmacro()

#set(SUPPRESS_FILES
#  "${libwebsockets_SOURCE_DIR}/lib/plat/unix/unix-misc.c"
#  "${libwebsockets_SOURCE_DIR}/lib/plat/unix/unix-sockets.c"
#  "${libwebsockets_SOURCE_DIR}/lib/plat/unix/unix-init.c"
#  "${libwebsockets_SOURCE_DIR}/lib/roles/ws/ops-ws.c"
#  "${libwebsockets_SOURCE_DIR}/lib/roles/listen/ops-listen.c"
#  "${libwebsockets_SOURCE_DIR}/lib/roles/raw-file/ops-raw-file.c"
#  "${libwebsockets_SOURCE_DIR}/lib/roles/h2/ops-h2.c"    
#)
#
#suppress_errors(FILES ${SUPPRESS_FILES} FLAGS "-Wno-unused-parameter" "-Wno-dev")

if (DEFINED LWS_LOG_MODE)
  if (${LWS_ERROR_MODE} STREQUAL "DEBUG")
    target_compile_definitions(websockets PRIVATE LWS_DEBUG=1)
  elseif(${LWS_ERROR_MODE} STREQUAL "VERBOSE")
    target_compile_definitions(websockets PRIVATE LWS_VERBOSE=1)   
  elseif (${LWS_ERROR_MODE} STREQUAL "ERROR")
    target_compile_definitions(websockets PRIVATE LWS_ERROR=1)
  else()
    message("All LWS message are suppressed")  
  endif()
endif()

set(GEMPYRE_WS_SOURCES 
    src/libwebsockets/server.cpp
    src/libwebsockets/lws_server.h
    )

set(GEMPYRE_WEBSOCKET_LIBRARY_NAME "libwebsocket")   

#set(UV_LIB_DIR ${BINARY_DIR})

if (WIN32)
  set(GEMPYRE_WS_LIB_NAME_CORE_RELEASE websockets_static)
  if(NOT IS_RELEASE)
      set(GEMPYRE_WS_LIB_NAME_CORE websockets_staticd)
  else()
      set(GEMPYRE_WS_LIB_NAME_CORE websockets_static)
  endif()
else()
  set(GEMPYRE_WS_LIB_NAME_CORE_RELEASE websockets)
  if(NOT IS_RELEASE)
      set(GEMPYRE_WS_LIB_NAME_CORE websocketsd)
  else()
      set(GEMPYRE_WS_LIB_NAME_CORE websockets)
  endif()
endif()

string(STRIP "${libwebsockets_BINARY_DIR}/lib" GEMPYRE_WS_LIB_PATH)
set(GEMPYRE_WS_LIB ${CMAKE_STATIC_LIBRARY_PREFIX}${GEMPYRE_WS_LIB_NAME_CORE}${CMAKE_STATIC_LIBRARY_SUFFIX})
if(WIN32)
  set(GEMPYRE_WS_LIB_FULL "${GEMPYRE_WS_LIB_PATH}/${GEMPYRE_WS_LIB}")
  set(GEMPYRE_WS_LIB_NAME "${GEMPYRE_WS_LIB}") # remove when uwebsockets are not used
  set(GEMPYRE_WS_LIB_OBJ  "libwebsocket")
  message("Using GEMPYRE_WS_LIB_PATH ${GEMPYRE_WS_LIB_PATH}")
  message("Using GEMPYRE_WS_LIB_OBJ ${GEMPYRE_WS_LIB_OBJ}")
  message("Using GEMPYRE_WS_LIB ${GEMPYRE_WS_LIB}")
  message("Using GEMPYRE_WS_LIB_FULL ${GEMPYRE_WS_LIB_FULL}")
else()
  set(GEMPYRE_WS_LIB_FULL "${GEMPYRE_WS_LIB_PATH}/${GEMPYRE_WS_LIB}")

endif()

set(CONNECTION_LIB_FULL ${GEMPYRE_WS_LIB_FULL})
