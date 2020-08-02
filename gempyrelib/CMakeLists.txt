cmake_minimum_required (VERSION 3.14)

# Must use GNUInstallDirs to install libraries into correct
# locations on all platforms.
include(GNUInstallDirs)
include(ExternalProject)
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
        set(UV_NAME "libuv")
        set(UV_DIR "${CMAKE_CURRENT_SOURCE_DIR}/extra/${UV_NAME}")

        if(CMAKE_BUILD_TYPE STREQUAL "Debug") #these actally pretty same, change if debug is needed.
             set(UV_LIB_DIR "${UV_DIR}/src/libuv-build/Debug")
        else()
            set(UV_LIB_DIR "${UV_DIR}/src/libuv-build/Release")
        endif()

        set(UV_LIB "${UV_LIB_DIR}/uv_a${CMAKE_STATIC_LIBRARY_SUFFIX}")

        if(NOT EXISTS ${UV_LIB})
            message("Lib uv not found: ${UV_LIB}")
            file(REMOVE_RECURSE ${UV_DIR}) #upon release debug cmake dont understand (?) target file
        endif()

        externalproject_add(libuv
            PREFIX ${UV_DIR}
            GIT_REPOSITORY https://github.com/libuv/libuv.git
           # GIT_TAG v1.34.0
            GIT_PROGRESS true
            CMAKE_ARGS -DLIBUV_BUILD_TESTS=OFF -DLIBUV_BUILD_BENCH=OFF -DCMAKE_BUILD_TYPE=Release
            UPDATE_DISCONNECTED false
            INSTALL_COMMAND ""
            UPDATE_COMMAND ""
            PATCH_COMMAND ""
            TEST_COMMAND ""
            BINARY_DIR "${UV_LIB_DIR}"
            )
        set(UV_SOURCES "${UV_DIR}/src/libuv")
        #add_custom_target(luv_lib DEPENDS ${UV_LIB})
        #add_dependencies(luv_lib libuv)
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

if(WIN32 OR ANDROID)
    message("Compile UV lib in")
else()
    add_custom_command(
        OUTPUT ${SOCKETS_DIR}/${SOCKETS_LIB_NAME}
        COMMAND make && mv ${SOCKETS_LIB_NAME} ${SOCKETS_DIR}/${SOCKETS_LIB_NAME}
        WORKING_DIRECTORY ${SOCKETS_SOURCES}
    )
    add_custom_target(makecmd DEPENDS ${SOCKETS_DIR}/${SOCKETS_LIB_NAME})
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
add_dependencies(libwebsockets libsockets)

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


if(NOT MSVC)
	find_package (Threads REQUIRED)
endif()

set(INCLUDES
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

if(WIN32 OR ANDROID)
    include_directories(
        ${INCLUDES}
        ${UV_SOURCES}/include
    )

    FILE(GLOB_RECURSE SOCKETS_SRC "${SOCKETS_SOURCES}/src/*.c") # its using plain Makefile so just add files IS easier
    FILE(GLOB_RECURSE SOCKETS_HRD "${SOCKETS_SOURCES}/src/*.h") # its using plain Makefile so just add files IS easier


    add_library(${PROJECT_NAME}
        ${SOCKETS_SRC}
        ${SOCKETS_HRD}
        ${GEMPYRE_SRC}
    )
else()
    include_directories(
        ${INCLUDES}
        )
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

add_dependencies(${PROJECT_NAME} libsockets)
add_dependencies(${PROJECT_NAME} libwebsockets)
add_dependencies(${PROJECT_NAME} libjson)

if (WIN32 OR ANDROID)
    add_dependencies(libsockets libuv)
    add_dependencies(${PROJECT_NAME} libuv)
else()
    add_dependencies(${PROJECT_NAME} makecmd)
    add_dependencies(makecmd libsockets)
endif()

#add_dependencies(${PROJECT_NAME} gempyrejs)

if(WIN32)
    message("uvdir ${UV_LIB}")
    set(WIN_LIB  psapi.lib userenv.lib iphlpapi.lib)
    target_link_libraries (${PROJECT_NAME}
        PRIVATE ${CMAKE_THREAD_LIBS_INIT}
        PRIVATE ${UV_LIB}
	PRIVATE ${WIN_LIB}
  #      PRIVATE ${SOCKETS_DIR}/${SOCKETS_LIB_NAME}
        )
elseif(ANDROID)
    message("uvdir ${UV_LIB}")
    #set(ANDROID_LIB  psapi.lib userenv.lib iphlpapi.lib)
    target_link_libraries (${PROJECT_NAME}
        PRIVATE ${CMAKE_THREAD_LIBS_INIT}
        PRIVATE ${UV_LIB}
  #	PRIVATE ${ANDROID_LIB}
  #      PRIVATE ${SOCKETS_DIR}/${SOCKETS_LIB_NAME}
        -lstdc++
        )
else()
    target_link_libraries (${PROJECT_NAME}
        PRIVATE ${CMAKE_THREAD_LIBS_INIT}
#    PRIVATE nlohmann_json::nlohmann_json #this wont work properly with find_package as it assumes the lib is somewhere
    PRIVATE ${SOCKETS_DIR}/${SOCKETS_LIB_NAME}
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
