#cmake_minimum_required (VERSION 3.18)


project (gempyre VERSION 1.5.1 LANGUAGES CXX C)
# Must use GNUInstallDirs to install libraries into correct
# locations on all platforms.
include(GNUInstallDirs)
include(ExternalProject)
include(FetchContent)
include(GenerateExportHeader)
include(../scripts/gempyre.cmake)

set(CMAKE_DEBUG_POSTFIX "d")
set(CMAKE_RELWITHDEBINFO_POSTFIX "rd")
set(CMAKE_MINSIZEREL_POSTFIX "rs")

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


if(MSYS OR MINGW)
    set(MinGW ON)
endif()    

string(TOUPPER ${CMAKE_BUILD_TYPE} CONFIG_NAME)
if (${CONFIG_NAME} STREQUAL "DEBUG")
    set(IS_RELEASE FALSE)
else()
    set(IS_RELEASE TRUE)
endif()

if( IS_RELEASE )
    if (${CONFIG_NAME} STREQUAL "RELWITHDEBINFO")
        message("Build is release with debug info")
    elseif (${CONFIG_NAME} STREQUAL "MINSIZEREL")
        message("Build is minimum size release")
    else()           
        message("Build is release")
    endif()    
else()
     message("Build is debug")
endif()

#message(WARNING "${IS_RELEASE} - ${CONFIG_NAME} - ${CMAKE_BUILD_TYPE} ")

option(BLEEDING_EDGE OFF)

#googletest-src release-1.10.0-612-g6c5c4554
#libjson v3.9.0-47-g350ff4f7c
#libdeflate v1.7-14-g4d3c0f0
#libwebsockets v19.0.0a5-3-g42d480e
#uSockets v0.7.1
#libsockets v0.7.1


if(BLEEDING_EDGE)
    message("Using bleeding edge libaries")
    set(LIB_JSON_VER master)
    set(LIB_ZLIB_VER master)
    set(LIB_PFD_VER main)
    set(LIB_PNG_VER master)
else()
    set(LIB_JSON_VER v3.11.3)
    set(LIB_ZLIB_VER v1.2.11)
    set(LIB_PFD_VER main v0.1.0)
    set(LIB_PNG_VER master) # no version
endif()

if(LINUX AND ${GEMPYRE_IS_MAIN_PROJECT})
    set(DOXYGEN TRUE)
    set(SPELL TRUE)
endif()    

if(USE_PYTHON_UI)
    message("Defaults to Python UI")
    add_compile_definitions(USE_PYTHON_UI=1)
else()
    message("Defaults to Browser UI")
endif()

if(EMSCRIPTEN)
    set(HAS_WEB_SOCKETS FALSE)
else()
    set(HAS_WEB_SOCKETS TRUE)    
endif()

# todo: use expternalproject_getproperty SOURCE DIR instead to simplify
set(EXTRA_DIR "${CMAKE_CURRENT_BINARY_DIR}/extra")

add_compile_definitions(GEMPYRE_PROJECT_VERSION=${PROJECT_VERSION} ${CMAKE_CXX_FLAGS})

if(WIN32)
    add_compile_definitions(WINDOWS_EXPORT WIN32)
    add_compile_options(-DWINDOWS_EXPORT)
    add_compile_options(-D_WIN32)

    if(NOT MSVC)
        add_compile_definitions(_WIN32_WINNT=0x0A00)  #win10, needed for mingw
    else()
        option(CMAKE_USE_WIN32_THREADS_INIT "using WIN32 threads" ON)
    endif()
endif()

if (WIN32 OR ANDROID OR RASPBERRY)
    set(COMPILE_SOCKETS_IN ON)
endif()
# end of general defintions

#if(USE_UWEBSOCKETS)
#    include(cmake/uwebsockets.cmake)
#else()
#   include(cmake/libwebsockets.cmake)
#    set(USE_LIBWEBSOCKETS TRUE)    
#endif()


set(JSON_BuildTests OFF)
set(JSON_Install OFF)
FetchContent_Declare(libjson
        GIT_REPOSITORY https://github.com/nlohmann/json.git
        GIT_TAG ${LIB_JSON_VER}
        GIT_PROGRESS ${HAS_PROGRESS}
        CMAKE_ARGS -DJSON_BuildTests:STRING=OFF
        )

FetchContent_MakeAvailable(libjson)
FetchContent_GetProperties(libjson SOURCE_DIR srcDirVar)        
set(SYSTEM_INCLUDES ${SYSTEM_INCLUDES} "${srcDirVar}/include")
set(JSON_DIR "${srcDirVar}/include" PARENT_SCOPE)

# store for testing json re-use
#set_property(TARGET libjson PROPERTY json_dir "${srcDirVar}/include")

if(HAS_WEB_SOCKETS)
    if(USE_LIBWEBSOCKETS)
        include(cmake/libwebsockets.cmake)    
    else()
        include(cmake/uwebsockets.cmake)
        set(USE_UWEBSOCKETS TRUE)
    endif()

    if(CLIENT_DIALOGS)
        set(DIALOG_SRC src/ui/ui_client.cpp)
    else()
        set(DIALOG_SRC src/ui/ui_pfd.cpp)
        message("skip libpfd version, use latest")
        externalproject_add(libpfd
            GIT_REPOSITORY https://github.com/samhocevar/portable-file-dialogs.git
            #GIT_TAG ${LIB_PFD_VER}
            GIT_TAG main
            GIT_PROGRESS ${HAS_PROGRESS}
            UPDATE_DISCONNECTED false
            BUILD_COMMAND ""
            CONFIGURE_COMMAND ""
            INSTALL_COMMAND ""
            UPDATE_COMMAND ""
            PATCH_COMMAND ""
            TEST_COMMAND ""
            )
        ExternalProject_Get_Property(libpfd SOURCE_DIR) 
        set(SYSTEM_INCLUDES ${SYSTEM_INCLUDES} "${SOURCE_DIR}")
    endif()    
endif()

if(USE_UWEBSOCKETS AND USE_LIBWEBSOCKETS)
    message(FATAL_ERROR "Confusing web socket configuration")
endif()

if (NOT HAS_WEB_SOCKETS)
    message("No web sockets")
elseif(USE_UWEBSOCKETS)
    message("Use uwebsocket")
elseif(USE_LIBWEBSOCKETS)
    message("Use libwebsockets")
else()
    message(FATAL_ERROR "I'm not sure what websocket library shall be used!")            
endif()

FetchContent_Declare(
    lodepng
    GIT_REPOSITORY https://github.com/lvandeve/lodepng.git
    #GIT_TAG ${LIB_PNG_VER}
    GIT_PROGRESS ${HAS_PROGRESS}
  )
  
FetchContent_MakeAvailable(lodepng)
FetchContent_GetProperties(lodepng SOURCE_DIR srcDirVar)

set(SYSTEM_INCLUDES ${SYSTEM_INCLUDES} "${srcDirVar}")
set(PNG_SRC 
    ${srcDirVar}/lodepng.cpp
    ${srcDirVar}/lodepng.h )

# suppress warnings for the PNG folder 
file(GLOB_RECURSE LODE_FILES LIST_DIRECTORIES FALSE ${srcDirVar}/*)
set_source_files_properties(${LODE_FILES} PROPERTIES COMPILE_FLAGS "-w")


if(NOT MSVC)
    set(CMAKE_CXX_FLAGS_DEBUG "-g")
	set(CMAKE_CXX_FLAGS_RELEASE "-O3")
else()
    if (MSVC_VERSION GREATER_EQUAL "1900")
        include(CheckCXXCompilerFlag)
        CHECK_CXX_COMPILER_FLAG("/std:c++17" _cpp_latest_flag_supported)
        if (_cpp_latest_flag_supported)
            add_compile_options("/std:c++17")
        endif()
    endif()
endif()

set(CMAKE_CXX_STANDARD 17)

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_definitions(_GLIBCXX_DEBUG)
  if(NOT EMSCRIPTEN)
    add_compile_options(-stdlib=libc++)
  endif()  
else()
 # set(CMAKE_EXE_LINKER_FLAGS "-static-libgcc -static-libstdc++")
 add_link_options(-lstdc++)
endif()

if(NOT (MSVC OR ANDROID))
    find_package (Threads REQUIRED)
endif()


set(INCLUDES
    include
    )

set(PUBLIC_HEADERS
    include/gempyre_utils.h
    include/gempyre_bitmap.h
    include/gempyre_types.h
)

set(INCLUDE_DIRECTORIES
    src
    )
 
if(NOT EMSCRIPTEN)
    
    set(INCLUDE_DIRECTORIES 
        ${INCLUDE_DIRECTORIES} 
        src/server
        src/ui)

    set(PUBLIC_HEADERS
        ${PUBLIC_HEADERS}
        include/gempyre.h
        include/gempyre_graphics.h
        include/gempyre_client.h
    )
    set(GEMPYRE_SRC
        src/server/semaphore.h
        src/server/gempyre_internal.h
        src/server/gempyre_internal.cpp
        src/server/core.h
        src/server/core.cpp
        src/server/server.h
        src/server/timer.h
        src/server/timer.cpp
        src/server/eventqueue.h
        src/server/server.cpp
        src/server/timequeue.h
        src/server/graphics.cpp
        src/server/element.cpp
        src/server/broadcaster.h
        ${DIALOG_SRC}
        ${GEMPYRE_WS_SOURCES}
    )
endif()

set(GEMPYRE_SRC
    ${PUBLIC_HEADERS}
    ${GEMPYRE_SRC}
    include/gempyre_utils.h
    include/gempyre_bitmap.h
    src/bitmap.cpp
    src/canvas_data.h
    src/canvas_data.cpp
    src/bitmap.cpp
    src/utils.cpp
    src/base64.cpp
    src/base64.h
    src/idlist.h
    src/json.cpp
    js/gempyre.js
    py/pyclient.py
    src/data.h
    src/data.cpp
    src/logging.cpp
    ${PNG_SRC}
    )

if(ANDROID)
    set(GEMPYRE_SRC ${GEMPYRE_SRC} src/android/android.cpp)
endif()

        
if(COMPILE_SOCKETS_IN)
    if(EMSCIPTEN)
        message(FATAL_ERROR "Sockets are not supported")
    endif()    
    FILE(GLOB_RECURSE SOCKETS_SRC "${SOCKETS_SOURCES}/src/*.c") # its using plain Makefile so just add files IS easier
    FILE(GLOB_RECURSE SOCKETS_HRD "${SOCKETS_SOURCES}/src/*.h") # its using plain Makefile so just add files IS easier
    if(MSVC)
        set_property(SOURCE ${SOCKETS_HRD} PROPERTY COMPILE_OPTIONS "/W0")
        set_property(SOURCE ${SOCKETS_SRC} PROPERTY COMPILE_OPTIONS "/W0")
    else()
        # suppress all warnings in globbed sources
        set_property(SOURCE ${SOCKETS_SRC} PROPERTY COMPILE_OPTIONS -w)
        set_property(SOURCE ${SOCKETS_HRD} PROPERTY COMPILE_OPTIONS -w)
    endif()
endif()

FILE(GLOB_RECURSE ALL_SRC LIST_DIRECTORIES FALSE "${CMAKE_CURRENT_BINARY_DIR}/gempyrelib/*.c*")
FILE(GLOB_RECURSE ALL_HDR LIST_DIRECTORIES FALSE "${CMAKE_CURRENT_BINARY_DIR}/gempyrelib/*.h*")
if(MSVC)
    set_source_files_properties(${ALL_SRC} PROPERTIES COMPILE_FLAGS "/w")
    set_source_files_properties(${ALL_SRC} PROPERTIES COMPILE_FLAGS "/w")
else()
    set_source_files_properties(${ALL_SRC} PROPERTIES COMPILE_FLAGS "-w")
    set_source_files_properties(${ALL_SRC} PROPERTIES COMPILE_FLAGS "-w")
endif()

if(DEFINED GEMPYRE_SHARED)
    set(LIB_TYPE SHARED)
    if(WIN32)
        if(MSVC)
            set(CMAKE_SHARED_LINKER_FLAGS  "${CMAKE_SHARED_LINKER_FLAGS} /NODEFAULTLIB:libcmt.lib /NODEFAULTLIB:libcmtd.lib /NODEFAULTLIB:msvcrtd.lib")
        else()
             set(CMAKE_SHARED_LINKER_FLAGS  "${CMAKE_SHARED_LINKER_FLAGS}")
        endif()
    endif()
else()
    set(LIB_TYPE STATIC)
    if(WIN32)
        if(MSVC)
            set(CMAKE_STATIC_LINKER_FLAGS  "${CMAKE_STATIC_LINKER_FLAGS} /NODEFAULTLIB:libcmt.lib /NODEFAULTLIB:libcmtd.lib /NODEFAULTLIB:msvcrtd.lib")
        else()
            set(CMAKE_STATIC_LINKER_FLAGS  "${CMAKE_STATIC_LINKER_FLAGS}")
        endif()
    endif()
endif()

if(ANDROID OR RASPBERRY)
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

if(PEDANTIC_WARNINGS)
    target_compile_options(${PROJECT_NAME} PRIVATE 
        -pedantic
        -Wall 
        -Wextra 
        -Wcast-align 
        -Wcast-qual 
        -Wctor-dtor-privacy 
        -Wdisabled-optimization 
        -Wformat=2 
        -Winit-self 
        -Wlogical-op 
        -Wmissing-declarations 
        -Wmissing-include-dirs 
        -Wnoexcept 
        -Wold-style-cast 
        -Woverloaded-virtual 
        -Wredundant-decls 
        -Wshadow 
        -Wsign-conversion 
        -Wsign-promo 
        -Wstrict-null-sentinel 
        -Wstrict-overflow=2 # 3-5 end up errorneous warning with Data::writeHeader 
        -Wswitch-default 
        -Wundef 
        -Werror 
        -Wno-unused
        # not applicable -Wabi
        # not an issue due c++17 copy elision -Waggregate-return
        -Wconversion 
        -Weffc++
        # There is some header only-code -Winline
        # Maintaining manual padding is pia, assumed not real benefit - -Wpadded
        -Wswitch-enum
        -Wunsafe-loop-optimizations
        -Wzero-as-null-pointer-constant
        -Wuseless-cast
    )
endif()

if(DOXYGEN)
    FetchContent_Declare(
        cmakedoc
        GIT_REPOSITORY https://github.com/mmertama/cmakedoc.git
        GIT_TAG        main
        GIT_PROGRESS ${HAS_PROGRESS}
    )
    FetchContent_MakeAvailable(cmakedoc)
    include(${cmakedoc_SOURCE_DIR}/cmakedoc.cmake)
    file(GLOB DOX_HEADERS include/*.h)
    set(CMAKEDOC_DOXYGEN_DOCUMENTS
        ${DOX_HEADERS}
    )
    add_doxygen(${PROJECT_NAME})


    if(SPELL)
    	# CMAKE_SOURCE_DIR referds to wrong directory with CMAKE_SOURCE_DIR on Github actions context!
        set(CMAKEDOC_SPELL_DICTIONARY ${CMAKE_CURRENT_SOURCE_DIR}/../aspell/gempyre_words.txt)
        set(CMAKEDOC_SPELL_EXCLUDE_DIRS
            build
            aspell
            modules
            res
            .git
            js
            py
            examples
            cmake-build-debug)
        add_spellcheck(${PROJECT_NAME})    
    endif()    
endif()

#if(APPLE)
#    if(USE_UWEBSOCKETS)
#        message("When uwebsockets is used then apple has issues with big messages")
#        target_compile_definitions(${PROJECT_NAME} PRIVATE ENSURE_SEND=65536)
#    endif()
#endif()

if(NOT MSVC)
    target_compile_options(${PROJECT_NAME}
        PRIVATE -Wall -Wextra -Wshadow -Wdouble-promotion -Wformat=2 -Wundef
        )
    
    target_compile_options(${PROJECT_NAME}
        PRIVATE -fno-common -Wconversion
        )   
endif()


if (LINUX OR RASPBERRY)
    if (NOT (SANITIZER OR SANITIZER_THREAD))
        # sanitizer makes stack bigger
        target_compile_options(${PROJECT_NAME}
            PRIVATE -fstack-usage -Wstack-usage=8192 -Wformat-truncation)
    endif()    
    if(NOT IS_RELEASE AND RASPBERRY)
        message("Add some Raspberry special debug flags")
        # -fsanitize=address
        # -fno-eliminate-unused-debug-symbols -gstatement-frontiers -ginline-points 
        # -ggdb  -fsanitize=address
        target_compile_options(${PROJECT_NAME}
            PRIVATE -g3 -fstack-protector-strong -fno-omit-frame-pointer) 
    endif()
endif()

#string(REPLACE " " ";" SYSTEM_INCLUDES_LIST ${SYSTEM_INCLUDES})

message("SYSTEM_INCLUDES: ${SYSTEM_INCLUDES}")
#message("SYSTEM_INCLUDES: ${SYSTEM_INCLUDES}")

target_include_directories( ${PROJECT_NAME}
    PUBLIC include
    PRIVATE ${INCLUDE_DIRECTORIES}
    )

include_directories(SYSTEM
    ${SYSTEM_INCLUDES}
    )

set_target_properties(${PROJECT_NAME} PROPERTIES DEBUG_POSTFIX ${CMAKE_DEBUG_POSTFIX})
set_target_properties(${PROJECT_NAME} PROPERTIES MINSIZEREL_POSTFIX ${CMAKE_MINSIZEREL_POSTFIX})
set_target_properties(${PROJECT_NAME} PROPERTIES RELWITHDEBINFO_POSTFIX ${CMAKE_RELWITHDEBINFO_POSTFIX})


if(CMAKE_BUILD_TYPE EQUAL "RELEASE")
    set(JSMIN MINIFY)
else()
    set(JSMIN "")
endif()

if(RASPBERRY OR WIN32)
    # RASPEBERRY is using chrome conf as a default instead of
    # pyclient.py
    # WINDOWS is using edge conf as a default instead of
    # pyclient.py - unfortunately there is no tabless mode?
    set(UI_APP "conf/gempyre_default.conf")
else()
    set(UI_APP "py/pyclient.py")     
endif()

gempyre_add_resources(
    PROJECT ${PROJECT_NAME} ${JSMIN} 
    TARGET src/generated/gempyre.js.h 
    SOURCES js/gempyre.js ${UI_APP})

add_custom_target(target ${PROJECT_NAME} PRE_BUILD
    COMMAND ${PYTHON3} -m py_compile py/pyclient.py
    VERBATIM
    )

#add_dependencies(${PROJECT_NAME} libjson)

if(NOT CLIENT_DIALOGS AND NOT EMSCRIPTEN)
    add_dependencies(${PROJECT_NAME} libpfd)
endif()


if(HAS_WEB_SOCKETS)
    socket_dependencies(${PROJECT_NAME})
endif()

#add_dependencies(${PROJECT_NAME} gempyrejs)

set_target_properties(${PROJECT_NAME} PROPERTIES gempyre_libs_path "${GEMPYRE_WS_LIB_PATH}")

if(WIN32)
    if(MSVC)
        set(WIN_LIB 
            psapi.lib
            userenv.lib
            iphlpapi.lib
            shlwapi.lib)
    else()
        # lot lot static linking make the binary pretty fat
        set(WIN_LIB 
            psapi.lib
            userenv.lib
            iphlpapi.lib
            ws2_32.lib
            shlwapi.lib
            -static-libgcc
            -static-libstdc++
            -Wl,-Bstatic -lstdc++ -lpthread -Wl,-Bdynamic)
    endif()


    set_target_properties(${PROJECT_NAME} PROPERTIES gempyre_libs "${GEMPYRE_WS_LIB_NAME};${WIN_LIB}")    
    set_target_properties(${PROJECT_NAME} PROPERTIES gempyre_libs_obj "${GEMPYRE_WS_LIB_OBJ};${WIN_LIB}")

    target_link_libraries (${PROJECT_NAME}
        PRIVATE ${CMAKE_THREAD_LIBS_INIT}
        PRIVATE ${GEMPYRE_WS_LIB}
        PUBLIC ${WIN_LIB}
        PRIVATE ${EXT_LIBS}
        )
elseif(ANDROID)
    set_target_properties(${PROJECT_NAME} PROPERTIES gempyre_libs ${GEMPYRE_WS_LIB_NAME})
    target_link_libraries (${PROJECT_NAME}
        PRIVATE -l${GEMPYRE_WS_LIB_NAME_CORE}
        PRIVATE -lstdc++
        PRIVATE ${EXT_LIBS}
        )
elseif(RASPBERRY)
    set_target_properties(${PROJECT_NAME} PROPERTIES gempyre_libs "${GEMPYRE_WS_LIB_NAME_CORE};${CMAKE_THREAD_LIBS_INIT}")
    target_link_libraries (${PROJECT_NAME}
        PRIVATE ${CMAKE_THREAD_LIBS_INIT}
        PRIVATE ${GEMPYRE_WS_LIB_NAME_CORE}
        PRIVATE -lstdc++
        PRIVATE ${EXT_LIBS}
        )        
else()
    set_target_properties(${PROJECT_NAME} PROPERTIES gempyre_libs  "${SOCKETS_LIB};${CMAKE_THREAD_LIBS_INIT}")
    target_link_libraries (${PROJECT_NAME}
        PRIVATE ${CMAKE_THREAD_LIBS_INIT}
        PRIVATE ${SOCKETS_LIB}
        PRIVATE -lstdc++
        PRIVATE ${EXT_LIBS}
        )
endif()

if(NO_EXPORT)
    message("Not installing")
else()
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
    set(LIBRARY_FULL_DIR "${LIBRARY_DIR}" CACHE PATH "" FORCE)
    set(INCLUDE_FULL_DIR "${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME}" CACHE PATH "" FORCE)
    if(UNIX AND NOT APPLE)
        set(PROJECT_CMAKE_DIR "${GEMPYRE_CONFIG_INSTALL_DIR}" CACHE PATH "" FORCE)
    else()
        set(PROJECT_CMAKE_DIR "${CMAKE_INSTALL_PREFIX}/${GEMPYRE_CONFIG_INSTALL_DIR}" CACHE PATH "" FORCE)
    endif()
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

    # all files in include and affiliates / include are exported
    if(NOT EMSCRIPTEN)    
        file(GLOB HEADERS ${CMAKE_SOURCE_DIR}/affiliates/include/*.h)
    endif()   

    set(HEADERS ${HEADERS} ${PUBLIC_HEADERS})

    set_target_properties(${PROJECT_NAME} PROPERTIES PUBLIC_HEADER "${HEADERS}")

    if(ANDROID)
        # Where to install for crosscompile? Maybe just use a ENV as this far
    else()
        if(DEFINED CONNECTION_FIND)
            set(SOCKET_CMAKE_PROJECT_CONFIG_FILE ${GEMPYRE_CMAKE_CONFIG_DIR}/${CONNECTION_FIND})
            configure_file(
                ${CMAKE_CURRENT_SOURCE_DIR}/cmake/${CONNECTION_FIND}.in
                ${SOCKET_CMAKE_PROJECT_CONFIG_FILE}
                @ONLY)
        endif()        

        install(
            TARGETS ${PROJECT_NAME}
            EXPORT GempyreTargets
            ARCHIVE DESTINATION ${LIBRARY_FULL_DIR}
            PUBLIC_HEADER DESTINATION ${INCLUDE_FULL_DIR}
            )

        # internal testing
        export(EXPORT GempyreTargets
        FILE  "${PROJECT_TARGETS_FILE}"
        NAMESPACE gempyre::
        )

        install(FILES ${CONNECTION_LIB_FULL}
            DESTINATION ${LIBRARY_FULL_DIR}
            OPTIONAL
            )


        if(MSVC)
            # I really wont get why on some setup above works, and sometimes this is needed 8-\ win
            install(FILES ${GEMPYRE_WS_LIB_FULL_R}
                DESTINATION ${LIBRARY_FULL_DIR}
                OPTIONAL
                )
        endif()

        if(USE_UWEBSOCKETS)
            set(SOCKET_CONFIG_FILE "${CMAKE_CURRENT_SOURCE_DIR}/cmake/uwebsockets_config.cmake")
        endif()

        if(HAS_WEB_SOCKETS)
            if(DEFINED CONNECTION_FIND)
                install(FILES
                    ${SOCKET_CMAKE_PROJECT_CONFIG_FILE}
                    DESTINATION ${PROJECT_CMAKE_DIR}
                    )
            else()
                message(WARNING "Project config is not installed - as CONNECTION_FIND is not defined!")        
            endif()
        endif()        

    install(FILES
        ${GEMPYRE_CMAKE_PROJECT_CONFIG_FILE}
        ${GEMPYRE_CMAKE_VERSION_CONFIG_FILE}
        ${CMAKE_SOURCE_DIR}/scripts/genStringResource.py
        ${CMAKE_SOURCE_DIR}/scripts/gempyre.cmake
        ${SOCKET_CONFIG_FILE}
        DESTINATION ${PROJECT_CMAKE_DIR}
        )

        install(DIRECTORY ${CMAKE_SOURCE_DIR}/scripts/jsmin-2.2.2
            DESTINATION ${PROJECT_CMAKE_DIR}
            )

        message("GempyreConfig.cmake is stored in ${PROJECT_CMAKE_DIR}")

        if(MSVC)
            install(FILES #CMAKE does not support (exp gen) static lib's pdb location
                "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.dir/${CMAKE_BUILD_TYPE}/${PROJECT_NAME}.pdb"
                DESTINATION ${LIBRARY_FULL_DIR}
                OPTIONAL
                )
        endif()

        if(WIN32)
        # make Windows easier to find the Gempyre
        set(ENV{gempyre_ROOT} ${PROJECT_CMAKE_DIR})
        endif()
    endif() # NOT NO_EXPORT
endif()

if(HAS_WEB_SOCKETS)
    if(NOT DEFINED GEMPYRE_WEBSOCKET_LIBRARY_NAME)
        message(FATAL_ERROR "Unknown Websocket library")
    else()
        message("Websocket library is " ${GEMPYRE_WEBSOCKET_LIBRARY_NAME})    
    endif()
endif()

