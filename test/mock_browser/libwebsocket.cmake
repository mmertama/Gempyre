include(FetchContent)

FetchContent_Declare(
  libwebsockets
  GIT_REPOSITORY https://libwebsockets.org/repo/libwebsockets
  GIT_TAG "v4.3.2"
  GIT_PROGRESS TRUE
)

set(LWS_WITHOUT_SERVER ON)
set(LWS_WITHOUT_CLIENT OFF)
option(LWS_WITHOUT_SERVER ON)
option(LWS_WITHOUT_CLIENT OFF)

option(DISABLE_WERROR ON)

if(WIN32)
  option(LWS_WITH_SYS_SMD OFF)
  set(LWS_WITH_SYS_SMD OFF)

  option(LWS_EXT_PTHREAD_LIBRARIES "")
  set(LWS_EXT_PTHREAD_LIBRARIES "")

  option(LWS_STATIC_PIC  ON)
  set(LWS_STATIC_PIC  ON)
    set(LWS_INSTALL_LIB_DIR ${CMAKE_BINARY_DIR}/foo1)
      set(LWS_INSTALL_CMAKE_DIR ${CMAKE_BINARY_DIR}/foo2)
      set(LWS_INSTALL_BIN_DIR ${CMAKE_BINARY_DIR}/foo3)
      set(LWS_INSTALL_EXAMPLES_DIR ${CMAKE_BINARY_DIR}/foo5)
  endif()


  set(LWS_WITHOUT_TESTAPPS ON)
  option(LWS_WITHOUT_TESTAPPS ON)
  set(LWS_WITH_MINIMAL_EXAMPLES ON)
  option(LWS_WITH_MINIMAL_EXAMPLES OFF)
  set(LWS_WITH_STATIC ON)
  option(LWS_WITH_STATIC ON)
  set(LWS_WITH_SHARED OFF)
  option(LWS_WITH_SHARED OFF)

  option(LWS_WITH_SSL OFF)
  set(LWS_WITH_NETWORK ON)
  option(LWS_WITH_NETWORK ON)
  set(LWS_WITH_SSL OFF)
  set(LWS_WITHOUT_BUILTIN_SHA1 OFF)

  option(LWS_ROLE_H1 ON)
  set(LWS_ROLE_H1 ON)

  option(LWS_ROLE_H2 ON)
  set(LWS_ROLE_H2 ON)

  set(LWS_WITH_HTTP2 ON)
  option(LWS_WITH_HTTP2 ON)

  option(LWS_WITH_DIR OFF)
  option(LWS_ROLE_WS ON)
  set(LWS_ROLE_WS ON)

  option(LWS_CTEST_INTERNET_AVAILABLE OFF)
  set(LWS_CTEST_INTERNET_AVAILABLE OFF)
  set(LWS_WITH_MINIMAL_EXAMPLES OFF)
  set(NOT LWS_WITHOUT_TESTAPPS OFF)

  # Windows
  option(LWS_SSL_CLIENT_USE_OS_CA_CERTS OFF)
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
