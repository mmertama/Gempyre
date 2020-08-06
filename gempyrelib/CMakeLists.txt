cmake_minimum_required (VERSION 3.14)

# Must use GNUInstallDirs to install libraries into correct
# locations on all platforms.
include(GNUInstallDirs)
include(ExternalProject)
include(FetchContent)
include(../scripts/addResource.cmake_script)

project (gempyre VERSION 0.2.0 LANGUAGES CXX C)
set(CMAKE_CXX_FLAGS "-DGEMPYRE_PROJECT_VERSION=${PROJECT_VERSION} ${CMAKE_CXX_FLAGS}")

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

if(WIN32)
    set(CMAKE_CXX_FLAGS "-DWINDOWS_EXPORT -D_WIN32 ${CMAKE_CXX_FLAGS}")
    add_compile_options(-DWINDOWS_EXPORT)
    add_compile_options(-D_WIN32)
endif()

if (WIN32 OR ANDROID)
        message("Need Libuv...")

        set(UV_DIR "${CMAKE_CURRENT_SOURCE_DIR}/extra/libuv")
        set(UV_LIB_DIR "${UV_DIR}/src/libuv-build")
		
        if (MSVC)
            if(CMAKE_BUILD_TYPE STREQUAL "Debug") #these actally pretty same, change if debug is needed.
                 set(UV_LIB "${UV_LIB_DIR}/Debug/uv_a${CMAKE_STATIC_LIBRARY_SUFFIX}")
            else()
                 set(UV_LIB "${UV_LIB_DIR}/Release/${CMAKE_STATIC_LIBRARY_PREFIX}uv_a${CMAKE_STATIC_LIBRARY_SUFFIX}")
            endif()
			message("MSVC builds artifacts upon release type")
         else()
             set(UV_LIB "${UV_LIB_DIR}/${CMAKE_STATIC_LIBRARY_PREFIX}uv_a${CMAKE_STATIC_LIBRARY_SUFFIX}")
         endif()

        externalproject_add(libuv
            PREFIX ${UV_DIR}
            GIT_REPOSITORY https://github.com/libuv/libuv.git
            GIT_TAG v1.34.0
            GIT_PROGRESS true
            BUILD_BYPRODUCTS ${UV_LIB}
            CMAKE_ARGS -DLIBUV_BUILD_TESTS=OFF -DLIBUV_BUILD_BENCH=OFF -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=${UV_LIB_DIR}
            UPDATE_DISCONNECTED false
            INSTALL_COMMAND  ${CMAKE_COMMAND} --build . --target install
            UPDATE_COMMAND ""
            PATCH_COMMAND ""
            TEST_COMMAND ""
            )

        set(UV_SOURCES "${UV_DIR}/src/libuv")
        set(INCLUDES "${UV_SOURCES}/include")
endif()


set(JSON_NAME "nlohmann_json")
set(JSON_DIR "${CMAKE_CURRENT_SOURCE_DIR}/extra/${JSON_NAME}")
externalproject_add(libjson
    PREFIX ${JSON_DIR}
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.7.0
    GIT_PROGRESS true
    UPDATE_DISCONNECTED false
    INSTALL_COMMAND ""
    UPDATE_COMMAND ""
    PATCH_COMMAND ""
    TEST_COMMAND ""
    CMAKE_ARGS -DJSON_BuildTests:STRING=OFF
    )


set(SOCKETS_NAME "uSockets")
set(SOCKETS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/extra/uNetworking/${SOCKETS_NAME}")

if(WIN32 OR ANDROID)
    # Sources are just globbed in the project, see later
    FetchContent_Declare(
      libsockets
      GIT_REPOSITORY https://github.com/uNetworking/uSockets.git
      GIT_TAG v0.3.2
    )
    
    FetchContent_GetProperties(libsockets)
    if(NOT libsockets_POPULATED)
        FetchContent_Populate(libsockets)
    endif()
	set(SOCKETS_SOURCES "${libsockets_SOURCE_DIR}")

else()
	set(SOCKETS_SOURCES "${SOCKETS_DIR}/src/libsockets")
    externalproject_add(libsockets
        PREFIX ${SOCKETS_DIR}
        GIT_REPOSITORY https://github.com/uNetworking/uSockets.git
        GIT_TAG v0.3.2
        GIT_PROGRESS true
        UPDATE_DISCONNECTED false
        BUILD_COMMAND  "" #make ${SOCKETS_SOURCES}/Makefile
        CONFIGURE_COMMAND ""
        INSTALL_COMMAND ""
        UPDATE_COMMAND ""
        PATCH_COMMAND ""
        TEST_COMMAND ""
        )

    set(SOCKETS_LIB_NAME uSockets${CMAKE_STATIC_LIBRARY_SUFFIX})
    set(SOCKETS_LIB ${SOCKETS_DIR}/${SOCKETS_LIB_NAME})
    #set(SOCKETS_LIB_NAME ${CMAKE_STATIC_LIBRARY_PREFIX}uSockets${CMAKE_STATIC_LIBRARY_SUFFIX})


    add_custom_command(
        OUTPUT ${SOCKETS_LIB}
        COMMAND make && mv ${SOCKETS_LIB_NAME} ${SOCKETS_LIB}
        WORKING_DIRECTORY ${SOCKETS_SOURCES}
        DEPENDS libsockets
    )
    add_custom_target(makecmd DEPENDS ${SOCKETS_LIB})
endif()


set(WEBSOCKETS_NAME "uWebSockets")
set(WEBSOCKETS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/extra/uNetworking/${WEBSOCKETS_NAME}")
externalproject_add(libwebsockets
    PREFIX ${WEBSOCKETS_DIR}
    GIT_REPOSITORY https://github.com/uNetworking/uWebSockets.git
    GIT_TAG v0.16.1
    GIT_PROGRESS true
    UPDATE_DISCONNECTED false
    BUILD_COMMAND ""
    CONFIGURE_COMMAND ""
    INSTALL_COMMAND ""
    UPDATE_COMMAND ""
    PATCH_COMMAND ""
    TEST_COMMAND ""
    )
set(WEBSOCKETS_SOURCES "${WEBSOCKETS_DIR}/src/libwebsockets")


find_path(ZLIB_H zlib.h PATHS
    ${CMAKE_C_IMPLICIT_INCLUDE_DIRECTORIES}
    ${CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES}
    /MinGW/MinGW/include #just for zlib.h , we search these if we are lucky
    )


if(NOT ZLIB_H)
    message("Retrieve zlib")
    set(ZLIB_NAME "zlib")
    set(ZLIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/extra/${ZLIB_NAME}")
    externalproject_add(zlib
        PREFIX ${ZLIB_DIR}
        GIT_REPOSITORY https://github.com/madler/zlib.git
        GIT_TAG v1.2.11
        GIT_PROGRESS true
        UPDATE_DISCONNECTED false
        BUILD_COMMAND ""
        CONFIGURE_COMMAND ""
        INSTALL_COMMAND ""
        UPDATE_COMMAND ""
        PATCH_COMMAND ""
        TEST_COMMAND ""
        )

    add_dependencies(libwebsockets zlib)
    set(ZLIB_H ${ZLIB_DIR}/src/zlib)
endif()

if(NOT MSVC)
    set(CMAKE_CXX_FLAGS_DEBUG "-g")
	set(CMAKE_CXX_FLAGS_RELEASE "-O3")
else()
    if (MSVC_VERSION GREATER_EQUAL "1900")
        include(CheckCXXCompilerFlag)
        CHECK_CXX_COMPILER_FLAG("/std:c++latest" _cpp_latest_flag_supported)
        if (_cpp_latest_flag_supported)
            add_compile_options("/std:c++latest")
        endif()
    endif()
endif()

set(CMAKE_CXX_STANDARD 17)

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}	-D_GLIBCXX_DEBUG")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
else()
 # set(CMAKE_EXE_LINKER_FLAGS "-static-libgcc -static-libstdc++")
 set(CMAKE_EXE_LINKER_FLAGS "-lstdc++")
endif()


set(CMAKE_CXX_FLAGS, "-DUWS_NO_ZLIB ${CMAKE_CXX_FLAGS}")
set(CMAKE_C_FLAGS, "-DUWS_NO_ZLIB ${CMAKE_C_FLAGS}")


if(NOT (MSVC OR ANDROID))
    find_package (Threads REQUIRED)
endif()

set(INCLUDES
    ${INCLUDES}
    include
    ${WEBSOCKETS_SOURCES}/src
    ${SOCKETS_SOURCES}/src
    include/generated
    ${JSON_DIR}/src/libjson/include
    ${ZLIB_H}
    )

set(GEMPYRE_SRC
    src/core.cpp
    src/server.cpp
    src/utils.cpp
    src/base64.cpp
    src/timer.cpp
    include/gempyre.h
    include/gempyre_utils.h
    include/gempyre_graphics.h
    src/server.h
    src/base64.h
    src/timer.h
    src/eventqueue.h
    src/semaphore.h
    src/idlist.h
    js/gempyre.js
    src/json.h
    src/json.cpp
    src/graphics.cpp
    src/element.cpp
    src/internal.h
    )

if(ANDROID)
    set(GEMPYRE_SRC ${GEMPYRE_SRC} src/android.cpp)
endif()

include_directories(
        ${INCLUDES}
        )
        



if(WIN32 OR ANDROID)
    FILE(GLOB_RECURSE SOCKETS_SRC "${SOCKETS_SOURCES}/src/*.c") # its using plain Makefile so just add files IS easier
    FILE(GLOB_RECURSE SOCKETS_HRD "${SOCKETS_SOURCES}/src/*.h") # its using plain Makefile so just add files IS easier
endif()

if(ANDROID)
    #openssl is not there
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DLIBUS_NO_SSL")
    list(REMOVE_ITEM SOCKETS_SRC "${SOCKETS_SOURCES}/src/ssl.c")
    add_library(${PROJECT_NAME}
        ${SOCKETS_SRC}
        ${SOCKETS_HRD}
        ${GEMPYRE_SRC}
    )
elseif(WIN32)
    add_library(${PROJECT_NAME}
        ${SOCKETS_SRC}
        ${SOCKETS_HRD}
        ${GEMPYRE_SRC}
    )
else()
    add_library(${PROJECT_NAME}
        ${GEMPYRE_SRC}
    )
endif()

if(CMAKE_BUILD_TYPE EQUAL "RELEASE")
    set(JSMIN MINIFY)
else()
    set(JSMIN "")
endif()

addResource(PROJECT ${PROJECT_NAME} ${JSMIN} TARGET src/generated/gempyre.js.h SOURCES js/gempyre.js)

add_dependencies(${PROJECT_NAME} libwebsockets)
add_dependencies(${PROJECT_NAME} libjson)

if (WIN32 OR ANDROID)
    #add_dependencies(libsockets libuv)
    add_dependencies(${PROJECT_NAME} libuv)
else()
    add_dependencies(libwebsockets libsockets)
    add_dependencies(${PROJECT_NAME} makecmd)
endif()

#add_dependencies(${PROJECT_NAME} gempyrejs)

if(WIN32)
    message("uvlib is set as:${UV_LIB}")
    set(WIN_LIB psapi.lib userenv.lib iphlpapi.lib)
    target_link_libraries (${PROJECT_NAME}
        PRIVATE ${CMAKE_THREAD_LIBS_INIT}
        PRIVATE ${UV_LIB}
        PRIVATE ${WIN_LIB}
        )
elseif(ANDROID)
    message("uvdir ${UV_LIB}")
    target_link_libraries (${PROJECT_NAME}
        PRIVATE ${UV_LIB}
        -lstdc++
        )
else()
    target_link_libraries (${PROJECT_NAME}
        PRIVATE ${CMAKE_THREAD_LIBS_INIT}
        PRIVATE ${SOCKETS_LIB}
        -lstdc++)
endif()

set(GEMPYRE_CONFIG_INSTALL_DIR        "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}" CACHE INTERNAL "")
set(GEMPYRE_TARGETS_EXPORT_NAME       "${PROJECT_NAME}")
set(GEMPYRE_CMAKE_CONFIG_TEMPLATE     "${CMAKE_CURRENT_SOURCE_DIR}/cmake/config.cmake.in")
set(GEMPYRE_CMAKE_CONFIG_DIR          "${CMAKE_CURRENT_SOURCE_DIR}")
set(GEMPYRE_CMAKE_VERSION_CONFIG_FILE "${GEMPYRE_CMAKE_CONFIG_DIR}/${PROJECT_NAME}ConfigVersion.cmake")
set(GEMPYRE_CMAKE_PROJECT_CONFIG_FILE "${GEMPYRE_CMAKE_CONFIG_DIR}/${PROJECT_NAME}Config.cmake")
set(GEMPYRE_CMAKE_PROJECT_TARGETS_FILE "${GEMPYRE_CMAKE_CONFIG_DIR}/${PROJECT_NAME}Targets.cmake")

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    ${GEMPYRE_CMAKE_VERSION_CONFIG_FILE} COMPATIBILITY SameMajorVersion
)

message("--> ${CMAKE_INSTALL_LIBDIR}")

configure_file(
    ${GEMPYRE_CMAKE_CONFIG_TEMPLATE}
    ${GEMPYRE_CMAKE_PROJECT_CONFIG_FILE}
    @ONLY
)


export(
    TARGETS ${PROJECT_NAME}
    FILE ${GEMPYRE_CMAKE_PROJECT_TARGETS_FILE}
    )

if(UNIX AND NOT APPLE)
    install(
       TARGETS ${PROJECT_NAME}
       ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
       LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
       COMPONENT library
       )
    install(
       FILES "src/gempyre.h"
       INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
       )
endif()


#if (LINUX)
#    set(CMAKE_INSTALL_PREFIX "/usr")
#endif()
#
#if (MACOS)
#    set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
#    set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
#endif()
#
#if (NOT WIN32)
#install(TARGETS ${PROJECT_NAME}
#        LIBRARY DESTINATION lib
#        FILES ${TELEX_HEADERS} DESTINATION include
#)
#else()
#
#endif()
