cmake_minimum_required (VERSION 3.14)

project (gempyre VERSION 0.3.4 LANGUAGES CXX C)
# Must use GNUInstallDirs to install libraries into correct
# locations on all platforms.
include(GNUInstallDirs)
include(ExternalProject)
include(FetchContent)
include(GenerateExportHeader)
include(../scripts/addResource.cmake)

set(CMAKE_DEBUG_POSTFIX "d")

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

message("compiler is ${CMAKE_CXX_COMPILER_ID}")
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "AppleClang")
  add_compile_definitions(COMPILER_CLANG)
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
  add_compile_definitions(COMPILER_GCC)
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel")
  add_compile_definitions(COMPILER_INTEL)
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
  add_compile_definitions(COMPILER_MSVC)
else()
    message(FATAL_ERROR "Cannot identify compiler: ${CMAKE_CXX_COMPILER_ID}")
endif()

string(TOLOWER ${CMAKE_BUILD_TYPE} LOWER)
if (${LOWER} STREQUAL debug)
    set(IS_RELEASE FALSE)
else()
    set(IS_RELEASE TRUE)
endif()

option(BLEEDING_EDGE OFF)

#googletest-src release-1.10.0-612-g6c5c4554
#libjson v3.9.0-47-g350ff4f7c
#libdeflate v1.7-14-g4d3c0f0
#libwebsockets v19.0.0a5-3-g42d480e
#uSockets v0.7.1
#libsockets v0.7.1

if(BLEEDING_EDGE)
    message("Using bleeding edge libaries")
    set(LIB_UV_VER  master)
    set(LIB_JSON_VER master)
    set(LIB_SOCKETS_VER master)
    set(LIB_WS_VER master)
    set(LIB_ZLIB_VER master)
else()
    set(LIB_UV_VER  v1.41.0)
    set(LIB_JSON_VER v3.9.0)
    set(LIB_SOCKETS_VER v0.7.1)
    set(LIB_WS_VER 0b97cc7)
    set(LIB_ZLIB_VER v1.2.11)
endif()

set(EXTRA_DIR "${CMAKE_CURRENT_BINARY_DIR}/extra")

set(CMAKE_CXX_FLAGS "-DGEMPYRE_PROJECT_VERSION=${PROJECT_VERSION} ${CMAKE_CXX_FLAGS}")

if(WIN32)
    set(CMAKE_CXX_FLAGS "-DWINDOWS_EXPORT -D_WIN32 ${CMAKE_CXX_FLAGS}")
    add_compile_options(-DWINDOWS_EXPORT)
    add_compile_options(-D_WIN32)
    if(NOT MSVC)
        add_compile_definitions(_WIN32_WINNT=0x0A00)  #win10, needed for mingw
    endif()
endif()


if (WIN32 OR ANDROID)
        set(UV_DIR "${EXTRA_DIR}/libuv")
        set(UV_LIB_DIR "${UV_DIR}/src/libuv-build")

        message("Need Libuv at ${UV_DIR} ")

        if(NOT ${IS_RELEASE})
            set(UVA_LIB_NAME_CORE uv_ad)
        else()
            set(UVA_LIB_NAME_CORE uv_a)
        endif()

        set(UVA_LIB_NAME ${CMAKE_STATIC_LIBRARY_PREFIX}${UVA_LIB_NAME_CORE}${CMAKE_STATIC_LIBRARY_SUFFIX})

        if (MSVC)
            if (NOT ${IS_RELEASE})
                    set(UV_LIB_FULL "${UV_LIB_DIR}/Debug/${UVA_LIB_NAME}")
            else()
                    set(UV_LIB_FULL "${UV_LIB_DIR}/Release/${UVA_LIB_NAME}")
            endif()
            message("MSVC builds artifacts upon release type at ${UV_LIB_FULL}")
         else()
             if(NOT ANDROID)
                  find_program(NINJA ninja.exe DOC "Ninja must be in path" REQUIRED)
             endif()
             set(UV_LIB_FULL "${UV_LIB_DIR}/${UVA_LIB_NAME}")
         endif()

        externalproject_add(libuv
            PREFIX ${UV_DIR}
            GIT_REPOSITORY https://github.com/libuv/libuv.git
            GIT_TAG ${LIB_UV_VER}
            GIT_PROGRESS true
            BUILD_BYPRODUCTS ${UV_LIB_FULL}
            CMAKE_ARGS
                -DLIBUV_BUILD_TESTS=OFF
                -DLIBUV_BUILD_BENCH=OFF
                -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
                -DCMAKE_INSTALL_PREFIX=${UV_LIB_DIR}
                -DCMAKE_DEBUG_POSTFIX=${CMAKE_DEBUG_POSTFIX}
            UPDATE_DISCONNECTED false
            INSTALL_COMMAND  ${CMAKE_COMMAND} --build . --target install
            UPDATE_COMMAND ""
            PATCH_COMMAND ""
            TEST_COMMAND ""
            )

        set(INCLUDES "${UV_DIR}/src/libuv/include")

#TODO: Same shit with usockets
#        add_library(uv_lib_interface STATIC IMPORTED)
#        set_target_properties(uv_lib_interface PROPERTIES
#            INTERFACE_LINK_LIBRARIES "${UV_LIB_FULL}"
#            INTERFACE_INCLUDE_DIRECTORIES "${UV_DIR}/src/libuv/include>"
#            )
#        set(UV_LIB uv_lib_interface)
endif()


set(JSON_NAME "nlohmann_json")
set(JSON_DIR "${EXTRA_DIR}/${JSON_NAME}")
externalproject_add(libjson
    PREFIX ${JSON_DIR}
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG ${LIB_JSON_VER}
    GIT_PROGRESS true
    UPDATE_DISCONNECTED false
    INSTALL_COMMAND ""
    UPDATE_COMMAND ""
    PATCH_COMMAND ""
    TEST_COMMAND ""
    CMAKE_ARGS -DJSON_BuildTests:STRING=OFF
    )


set(SOCKETS_NAME "uSockets")
set(SOCKETS_DIR "${EXTRA_DIR}/uNetworking/${SOCKETS_NAME}")

if(WIN32 OR ANDROID)
    # Sources are just globbed in the project, see later
    FetchContent_Declare(
      libsockets
      GIT_REPOSITORY https://github.com/uNetworking/uSockets.git
      GIT_TAG ${LIB_SOCKETS_VER}
    )
    
    FetchContent_GetProperties(libsockets)
    if(NOT libsockets_POPULATED)
        FetchContent_Populate(libsockets)
    endif()
    set(SOCKETS_SOURCES "${libsockets_SOURCE_DIR}")

else()

    set(SOCKETS_SOURCES "${SOCKETS_DIR}/src/libsockets")
    set(SOCKETS_INCLUDE_PATH "${CMAKE_PREFIX_PATH}/${SOCKETS_SOURCES}/src")
    set(SOCKETS_LIBRARY_PATH "${CMAKE_PREFIX_PATH}/${SOCKETS_DIR}/src")

    externalproject_add(libsockets
        PREFIX ${SOCKETS_DIR}
        GIT_REPOSITORY https://github.com/uNetworking/uSockets.git
        GIT_TAG ${LIB_SOCKETS_VER}
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
    set(SOCKETS_LIB_FULL ${SOCKETS_DIR}/${SOCKETS_LIB_NAME})
    set(SOCKETS_LIB ${SOCKETS_DIR}/${SOCKETS_LIB_NAME})

    #file(GLOB SOCKETS_OBJ "${SOCKETS_SOURCES}/*.o") # get bored with depencies, maybe fix later

    add_custom_command(
        OUTPUT ${SOCKETS_LIB}
        COMMAND make && mv ${SOCKETS_LIB_NAME} ${SOCKETS_LIB}
        WORKING_DIRECTORY ${SOCKETS_SOURCES}
        DEPENDS libsockets
    )
    add_custom_target(makecmd DEPENDS ${SOCKETS_LIB})
endif()


set(WEBSOCKETS_NAME "uWebSockets")
set(WEBSOCKETS_DIR "${EXTRA_DIR}/uNetworking/${WEBSOCKETS_NAME}")
externalproject_add(libwebsockets
    PREFIX ${WEBSOCKETS_DIR}
    GIT_REPOSITORY https://github.com/uNetworking/uWebSockets.git
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
set(WEBSOCKETS_SOURCES "${WEBSOCKETS_DIR}/src/libwebsockets")


find_path(ZLIB_H zlib.h PATHS
    ${CMAKE_C_IMPLICIT_INCLUDE_DIRECTORIES}
    ${CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES}
    /MinGW/MinGW/include #just for zlib.h , we search these if we are lucky
    )


if(NOT ZLIB_H)
    message("Retrieve zlib")
    set(ZLIB_NAME "zlib")
    set(ZLIB_DIR "${EXTRA_DIR}/${ZLIB_NAME}")
    externalproject_add(zlib
        PREFIX ${ZLIB_DIR}
        GIT_REPOSITORY https://github.com/madler/zlib.git
        GIT_TAG ${LIB_ZLIB_VER}
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
    src/json.cpp
    src/graphics.cpp
    src/element.cpp
    src/internal.h
    )

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "AppleClang")
    # bool type is also used as reference, even the compiler warns, therefore we suppress
    set_source_files_properties(${CMAKE_CURRENT_SOURCE_DIR}/src/json.cpp PROPERTIES COMPILE_FLAGS "-Wrange-loop-analysis")
    # with syslog stuff
    set_source_files_properties(${CMAKE_CURRENT_SOURCE_DIR}/utils.cpp PROPERTIES COMPILE_FLAGS "-Wformat-security")
    set_target_properties(libwebsockets PROPERTIES COMPILE_FLAGS "-Wall -Wextra")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
  set_target_properties(libwebsockets PROPERTIES COMPILE_FLAGS "-Wall -Wextra")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel")
  set_target_properties(libwebsockets PROPERTIES COMPILE_FLAGS "-Wall -Wextra")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
  set_target_properties(libwebsockets PROPERTIES COMPILE_FLAGS "/w")
else()
    message(FATAL_ERROR "Cannot identify compiler: ${CMAKE_CXX_COMPILER_ID}")
endif()

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

if(DEFINED GEMPYRE_SHARED)
    set(LIB_TYPE SHARED)
else()
    set(LIB_TYPE STATIC)
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
    add_library(${PROJECT_NAME} ${LIB_TYPE}
        ${SOCKETS_SRC}
        ${SOCKETS_HRD}
        ${GEMPYRE_SRC}
    )
else()
    add_library(${PROJECT_NAME} ${LIB_TYPE}
        ${GEMPYRE_SRC}
    )
endif()

set_target_properties(${PROJECT_NAME} PROPERTIES DEBUG_POSTFIX ${CMAKE_DEBUG_POSTFIX})

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
    message("uvlib is set as:'${UV_LIB_FULL}'")
    if(MSVC)
        set(WIN_LIB psapi.lib userenv.lib iphlpapi.lib)
    else()
        set(WIN_LIB psapi.lib userenv.lib iphlpapi.lib ws2_32.lib)
    endif()
    set_target_properties(${PROJECT_NAME} PROPERTIES gempyre_libs "${UVA_LIB_NAME};${WIN_LIB}")
    set_target_properties(${PROJECT_NAME} PROPERTIES gempyre_libs_path "${UV_LIB_DIR}")
    target_link_libraries (${PROJECT_NAME}
        PRIVATE ${CMAKE_THREAD_LIBS_INIT}
        #PRIVATE ${UV_LIB}
        PRIVATE ${WIN_LIB}
        )
elseif(ANDROID)
    message("uvdir ${UV_LIB}")
    set_target_properties(${PROJECT_NAME} PROPERTIES gempyre_libs ${UV_LIB})
    target_link_libraries (${PROJECT_NAME}
        PRIVATE ${UV_LIB}
        PRIVATE -lstdc++
        )
else()
    set_target_properties(${PROJECT_NAME} PROPERTIES gempyre_libs "${SOCKETS_LIB};${CMAKE_THREAD_LIBS_INIT}")
    target_link_libraries (${PROJECT_NAME}
        PRIVATE ${CMAKE_THREAD_LIBS_INIT}
        PRIVATE ${SOCKETS_LIB}
        PRIVATE -lstdc++)
endif()

#if(WIN32)
#    if(MSVC)
#        set(PROJECT_TARGETS_FILE ${PROJECT_NAME}Targets_vc.cmake)
#    else()
#        set(PROJECT_TARGETS_FILE ${PROJECT_NAME}Targets_gw.cmake)
#    endif()
#else()
#    set(PROJECT_TARGETS_FILE ${PROJECT_NAME}Targets.cmake)
#endif()

set(GEMPYRE_CONFIG_INSTALL_DIR        "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}")
set(GEMPYRE_TARGETS_EXPORT_NAME       "${PROJECT_NAME}")
set(GEMPYRE_CMAKE_CONFIG_TEMPLATE     "${CMAKE_CURRENT_SOURCE_DIR}/cmake/config.cmake.in")
set(GEMPYRE_CMAKE_CONFIG_DIR          "${CMAKE_CURRENT_BINARY_DIR}")
set(GEMPYRE_CMAKE_VERSION_CONFIG_FILE   "${GEMPYRE_CMAKE_CONFIG_DIR}/${PROJECT_NAME}ConfigVersion.cmake")
set(GEMPYRE_CMAKE_PROJECT_CONFIG_FILE   "${GEMPYRE_CMAKE_CONFIG_DIR}/${PROJECT_NAME}Config.cmake")
#set(GEMPYRE_CMAKE_PROJECT_TARGETS_FILE  "${GEMPYRE_CMAKE_CONFIG_DIR}/${PROJECT_TARGETS_FILE}")

generate_export_header(${PROJECT_NAME})

#set(RESOURCE_SCRIPTS_DIR "${CMAKE_PREFIX_PATH}/${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}")
set(LIBRARY_DIR ${CMAKE_INSTALL_LIBDIR}/${PROJECT_NAME})
set(LIBRARY_FULL_DIR ${CMAKE_INSTALL_PREFIX}/${LIBRARY_DIR})
set(INCLUDE_FULL_DIR ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME})
set(PROJECT_CMAKE_DIR "${CMAKE_INSTALL_PREFIX}/${GEMPYRE_CONFIG_INSTALL_DIR}")
get_target_property(TARGET_LIBRARY_NAME ${PROJECT_NAME} OUTPUT_NAME)

include(CMakePackageConfigHelpers)
configure_package_config_file(
    ${GEMPYRE_CMAKE_CONFIG_TEMPLATE}
    ${GEMPYRE_CMAKE_PROJECT_CONFIG_FILE}
    INSTALL_DESTINATION ${GEMPYRE_CONFIG_INSTALL_DIR}
    )
write_basic_package_version_file(
    ${GEMPYRE_CMAKE_VERSION_CONFIG_FILE}
    COMPATIBILITY SameMajorVersion
    )



file(GLOB HEADERS include/*.h  ${CMAKE_SOURCE_DIR}/affiliates/include/*.h)

set_target_properties(${PROJECT_NAME} PROPERTIES PUBLIC_HEADER "${HEADERS}")

if(ANDROID)
    # Where to install for crosscompile? Maybe just use a ENV as this far
else()
    if(WIN32)
        set(CONNECTION_FIND Finduva.cmake)
        set(CONNECTION_LIB_FULL ${UV_LIB_FULL})
    else()
        set(CONNECTION_FIND FinduSockets.cmake)
        set(CONNECTION_LIB_FULL ${SOCKETS_LIB_FULL})
    endif()

    set(SOCKET_CMAKE_PROJECT_CONFIG_FILE ${GEMPYRE_CMAKE_CONFIG_DIR}/${CONNECTION_FIND})
    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/cmake/${CONNECTION_FIND}.in
        ${SOCKET_CMAKE_PROJECT_CONFIG_FILE}
        @ONLY)

    install(
       TARGETS ${PROJECT_NAME}
       EXPORT GempyreTargets
       ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}/${PROJECT_NAME}
       PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME}
       )

       # internal testing
   export(EXPORT GempyreTargets
     FILE  "${PROJECT_TARGETS_FILE}"
     NAMESPACE gempyre::
     )

     #external 
   #install(EXPORT GempyreTargets
   #    NAMESPACE gempyre::
   #    FILE ${PROJECT_TARGETS_FILE}
   #    DESTINATION "${GEMPYRE_CONFIG_INSTALL_DIR}"
   #    )

       install(FILES ${CONNECTION_LIB_FULL}
            DESTINATION ${LIBRARY_DIR}
            )

    install(FILES
            ${SOCKET_CMAKE_PROJECT_CONFIG_FILE}
            DESTINATION "${GEMPYRE_CONFIG_INSTALL_DIR}"
            )

   install(FILES
       ${GEMPYRE_CMAKE_PROJECT_CONFIG_FILE}
       ${GEMPYRE_CMAKE_VERSION_CONFIG_FILE}
       ${CMAKE_SOURCE_DIR}/scripts/genStringResource.py
       ${CMAKE_SOURCE_DIR}/scripts/addResource.cmake
       DESTINATION "${GEMPYRE_CONFIG_INSTALL_DIR}"
       )

    install(DIRECTORY ${CMAKE_SOURCE_DIR}/scripts/jsmin-2.2.2
        DESTINATION "${GEMPYRE_CONFIG_INSTALL_DIR}"
        )

    if(MSVC)
        install(FILES #CMAKE does not support (exp gen) static lib's pdb location
            "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.dir/${CMAKE_BUILD_TYPE}/${PROJECT_NAME}.pdb"
            DESTINATION ${LIBRARY_DIR}
            OPTIONAL
            )
    endif()

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
