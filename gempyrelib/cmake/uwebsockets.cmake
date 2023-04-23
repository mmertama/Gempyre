
if(BLEEDING_EDGE)  
    set(LIB_UV_VER  master)
    set(LIB_SOCKETS_VER master)
    set(LIB_WS_VER master)
else()
    set(LIB_UV_VER  v1.44.2)
    set(LIB_SOCKETS_VER v0.7.1)
    set(LIB_WS_VER v20.13.0)
endif()

# LIB UV

if (COMPILE_SOCKETS_IN)
    # NOTE: lib UV is linking only MD, therefore Gempyre to be linked MD, and thus MT wont work here either, sigh
    # maybe use Fetch command instead of Externalproject add and then override flags
    externalproject_add(libuv
    GIT_REPOSITORY https://github.com/libuv/libuv.git
    GIT_TAG ${LIB_UV_VER}
    GIT_PROGRESS true
    CMAKE_ARGS
        -DLIBUV_BUILD_TESTS=OFF
        -DLIBUV_BUILD_BENCH=OFF
        #-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
        -DCMAKE_BUILD_TYPE=Release
        -DCMAKE_DEBUG_POSTFIX=${CMAKE_DEBUG_POSTFIX}
    UPDATE_DISCONNECTED false
    INSTALL_COMMAND "" # libuv wont always respect install prefix :-(
    UPDATE_COMMAND ""
    PATCH_COMMAND ""
    TEST_COMMAND ""
    )

    ExternalProject_Get_Property(libuv SOURCE_DIR)
    set(UV_SRC_DIR ${SOURCE_DIR})

    ExternalProject_Get_Property(libuv BINARY_DIR)
    set(UV_LIB_DIR ${BINARY_DIR})
                                       
    if(NOT IS_RELEASE)
        set(UVA_LIB_NAME_CORE uv_ad)
    else()
        set(UVA_LIB_NAME_CORE uv_a)
    endif()

    set(UVA_LIB_NAME ${CMAKE_STATIC_LIBRARY_PREFIX}${UVA_LIB_NAME_CORE}${CMAKE_STATIC_LIBRARY_SUFFIX})

    if (MSVC)
        message(WARNING "CMAKE_MAKE_BUILD_TYPE wont work with msvc, use cmake --build . --config Release or Debug")
        set(UV_LIB_FULL "${UV_LIB_DIR}/${UVA_LIB_NAME}")
        message("gempyre-libuv: MSVC builds artifacts upon release type at ${UV_LIB_FULL}")
    else()
        if(MinGW)
            find_program(NINJA ninja.exe DOC "Ninja must be in path" REQUIRED)
             # hack hack
            #if(NOT IS_RELEASE)
            #    find_file(FUV_LIB ${UVA_LIB_NAME} PATHS ${UV_LIB_DIR})
            #    if(NOT FUV_LIB)
            #        message(WARNING "For reason or other ${UV_LIB_DIR}/${UVA_LIB_NAME} is not found, copy from release")
            #        configure_file("${UV_LIB_DIR}/libuv_a.a" "${UV_LIB_DIR}/libuv_ad.a" COPYONLY)
            #        find_file(FUV_LIB ${UVA_LIB_NAME} PATHS ${UV_LIB_DIR})
            #        if(NOT FUV_LIB)
            #            message(FATAL_ERROR "For reason or other ${UV_LIB_DIR}/${UVA_LIB_NAME} is not found")
            #        endif()
            #    endif()
            #endif() 
        endif()
        get_filename_component(UV_LIB_FULL "${UV_LIB_DIR}/${UVA_LIB_NAME}" ABSOLUTE)
    endif()

    set(SYSTEM_INCLUDES "${UV_SRC_DIR}/include")

    message(STATUS "gempyre-libuv lib: ${UV_LIB_FULL} inc: ${SYSTEM_INCLUDES}")

endif()

# LIB UV end

# LIB Sockets

if(COMPILE_SOCKETS_IN)
    # Sources are just globbed in the project, see later

    FetchContent_Declare(
      libsockets
      GIT_REPOSITORY https://github.com/uNetworking/uSockets.git
      GIT_TAG ${LIB_SOCKETS_VER}
    )
    
    FetchContent_MakeAvailable(libsockets)

    FetchContent_GetProperties(libsockets SOURCE_DIR srcDirVar)


    set(SOCKETS_SOURCES ${srcDirVar})
    set(SYSTEM_INCLUDES "${SYSTEM_INCLUDES} ${srcDirVar}/src")

else()
    externalproject_add(libsockets
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

    ExternalProject_Get_Property(libsockets SOURCE_DIR)
    set(SOCKETS_SOURCES ${SOURCE_DIR})

    ExternalProject_Get_Property(libsockets BINARY_DIR)
    set(SOCKETS_LIB_DIR ${BINARY_DIR})

    set(SOCKETS_LIB_NAME uSockets${CMAKE_STATIC_LIBRARY_SUFFIX})
    set(SOCKETS_LIB_FULL ${SOCKETS_LIB_DIR}/${SOCKETS_LIB_NAME})
    set(SOCKETS_LIB ${SOCKETS_LIB_DIR}/${SOCKETS_LIB_NAME})

    #file(GLOB SOCKETS_OBJ "${SOCKETS_SOURCES}/*.o") # get bored with depencies, maybe fix later

    add_custom_command(
        OUTPUT ${SOCKETS_LIB}
        COMMAND make && mv ${SOCKETS_LIB_NAME} ${SOCKETS_LIB}
        WORKING_DIRECTORY ${SOCKETS_SOURCES}
        DEPENDS libsockets
    )
    add_custom_target(makecmd DEPENDS ${SOCKETS_LIB})

    ExternalProject_Get_Property(libsockets SOURCE_DIR)
    set(SYSTEM_INCLUDES "${SYSTEM_INCLUDES} ${SOURCE_DIR}/src")
  
endif()


# LIB Sockets end

# LIB uWebSockets

externalproject_add(libwebsockets
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

ExternalProject_Get_Property(libwebsockets SOURCE_DIR)
set(WEBSOCKETS_SOURCES ${SOURCE_DIR})
set(SYSTEM_INCLUDES "${SYSTEM_INCLUDES} ${SOURCE_DIR}/src")    

#set_property(DIRECTORY ${WEBSOCKETS_SOURCES}
#    PROPERTY COMPILE_DEFINITIONS "DISABLE_WERROR=ON")

# LIB uWebSockets end

#LIB Zlib 

if(NOT ZLIB_H)
    message("Retrieve zlib")
    externalproject_add(zlib
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
    ExternalProject_Get_Property(zlib SOURCE_DIR)
    set(ZLIB_SOURCES ${SOURCE_DIR})  
    set(SYSTEM_INCLUDES "${SYSTEM_INCLUDES} ${ZLIB_SOURCES}/src/zlib")
endif()

# LIB ZLib end

find_path(ZLIB_H zlib.h PATHS
    ${CMAKE_C_IMPLICIT_INCLUDE_DIRECTORIES}
    ${CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES}
    /MinGW/MinGW/include #just for zlib.h , we search these if we are lucky
    )
 

add_compile_definitions(UWS_NO_ZLIB)


if(ANDROID OR RASPBERRY)
    #openssl is not there
    add_compile_definitions(DLIBUS_NO_SSL)
    list(REMOVE_ITEM SOCKETS_SRC "${SOCKETS_SOURCES}/src/ssl.c")
endif()    

if(WIN32)
    message("uvlib is set as:'${UV_LIB_FULL}'")
    set(GEMPYRE_WS_LIB_NAME "${UVA_LIB_NAME}")
    set(GEMPYRE_WS_LIB_OBJ  "${UVA_LIB}")
    set(GEMPYRE_WS_LIB_PATH "${UV_LIB_DIR}")
    set(GEMPYRE_WS_LIB "${UV_LIB}")
    set(GEMPYRE_WS_LIB_FULL "${UV_LIB_FULL}")
endif()

if(MSVC)
    set(GEMPYRE_WS_LIB_FULL_R ${UV_LIB_FULL_R})
endif()

if(ANDROID)
    message("uvdir ${UVA_LIB_NAME}")
    set(GEMPYRE_WS_LIB_NAME_CORE "${UVA_LIB_NAME_CORE}")
    set(GEMPYRE_WS_LIB_PATH "${UV_LIB_DIR}")
endif()

if(RASPBERRY)
    message("uvlib is set as:'${UV_LIB_FULL}'")
    set(GEMPYRE_WS_LIB_FULL "${UV_LIB_FULL}")
    set(GEMPYRE_WS_LIB_NAME_CORE "${UVA_LIB_NAME_CORE}")
endif()


if(WIN32)
    set(CONNECTION_FIND Finduva.cmake)
    set(CONNECTION_LIB_FULL ${UV_LIB_FULL})
elseif(RASPBERRY)
    set(CONNECTION_FIND Finduva.cmake)
    set(CONNECTION_LIB_FULL ${UV_LIB_FULL})
elseif(ANDROID)
else()
    set(CONNECTION_FIND FinduSockets.cmake)
    set(CONNECTION_LIB_FULL ${SOCKETS_LIB_FULL})
endif()

 


macro(socket_dependencies TARGET)
    target_compile_definitions(${TARGET} PRIVATE USE_UWEBSOCKETS)
    target_include_directories(${TARGET} PRIVATE src/uwebsockets)
    add_dependencies(${TARGET} libwebsockets)
    if (COMPILE_SOCKETS_IN)
        #add_dependencies(libsockets libuv)
        add_dependencies(${TARGET} libuv)
    else()
        add_dependencies(libwebsockets libsockets)
        add_dependencies(${TARGET} makecmd)
    endif()
endmacro()

set(GEMPYRE_WS_SOURCES 
    src/uwebsockets/broadcaster.h
    src/uwebsockets/server.cpp
    src/uwebsockets/uws_server.h
    )
