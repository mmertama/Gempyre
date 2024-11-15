cmake_minimum_required (VERSION 3.25)
project(gempyre LANGUAGES CXX)

# enable ctests, starting from the top build folder
include(CTest)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_CURRENT_SOURCE_DIR})
    set(GEMPYRE_IS_MAIN_PROJECT TRUE)
else()
    set(GEMPYRE_IS_MAIN_PROJECT FALSE)
endif()        

string(TOLOWER ${CMAKE_BUILD_TYPE} LOWER)
if (${LOWER} STREQUAL debug)
    set(IS_RELEASE FALSE)
    add_compile_definitions(GEMPYRE_IS_DEBUG)
else()
    set(IS_RELEASE TRUE)
endif()

if(NOT EXISTS ${GEMPYRE_DIR})
    if(EXISTS $ENV{GEMPYRE_DIR})
        set(GEMPYRE_DIR $ENV{GEMPYRE_DIR})
    else()
        if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/scripts)
            set(GEMPYRE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
        else()
            message(FATAL_ERROR "GEMPYRE_DIR is not set neither GEMPYRE_DIR Environment variable is not pointing to Gempyre dir")
        endif()
    endif()
endif()

# NOTE: lib UV is linking only MD, therefore Gempyre to be linked MD, and thus MT wont work here either, sigh
# maybe use Fetch command instead of Externalproject add and then override flags

if(WIN32)
    add_compile_definitions(WINDOWS_OS)
    add_compile_options(-D_WIN32)
    add_compile_options(-DWINDOWS_OS)
    add_compile_options(-DLIBUS_NO_SSL)
    add_compile_options(-DNOMINMAX)
    set(CMAKE_DEBUG_POSTFIX d)
    set(CMAKE_RELWITHDEBINFO_POSTFIX rd)
    set(CMAKE_MINSIZEREL_POSTFIX rs)
else()
    add_compile_options(-Wall -Wextra)
endif()

if(APPLE)
    add_compile_definitions(MAC_OS)
    if (NOT DEFINED NATIVE_ARCHITECURE)
        set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "Universal binary" FORCE)
    endif()    
elseif(RASPBERRY)
    add_compile_definitions(UNIX_OS DRASPBERRY_OS)
    add_compile_options(-Wno-psabi)
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
    add_compile_options(-fPIC)
elseif(ANDROID)
    add_compile_definitions(ANDROID_OS)
elseif(UNIX)
    add_compile_definitions(UNIX_OS)
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
    add_compile_options(-fPIC)
endif()

set(USE_DEFAULT_SUBPROJECTS ${GEMPYRE_IS_MAIN_PROJECT})
set(USE_DEFAULT_PYTHON ON)

if(EMSCRIPTEN)
    message("Target is Emscripten")
    add_compile_definitions(WASM)
    set(USE_DEFAULT_SUBPROJECTS OFF)
    set(USE_DEFAULT_PYTHON OFF)
endif()

option(CI_ACTIONS "Github Actions build" OFF)

if(CI_ACTIONS)
    add_compile_options(-DCI_ACTIONS)
    message(WARNING "This is for Github actions")
endif()

if(CI_ACTIONS)
    set(HAS_PROGRESS FALSE)
else()
    set(HAS_PROGRESS TRUE)
endif()    

if(SANITIZER)
    set(ASAN_FLAGS
        -fsanitize=address
        -fsanitize=pointer-compare
        -fsanitize=pointer-subtract
        -fsanitize=leak
        -fsanitize=undefined
        -fsanitize=shift
        -fsanitize=shift-exponent
        -fsanitize=shift-base
        -fsanitize=integer-divide-by-zero
        -fsanitize=unreachable
        -fsanitize=vla-bound
        -fsanitize=null
        -fsanitize=return
        -fsanitize=signed-integer-overflow
        -fsanitize=bounds
        -fsanitize=bounds-strict
        -fsanitize=alignment
        -fsanitize=object-size
        -fsanitize=float-divide-by-zero
        -fsanitize=float-cast-overflow
        -fsanitize=nonnull-attribute
        -fsanitize=returns-nonnull-attribute
        -fsanitize=bool
        -fsanitize=enum
        -fsanitize=vptr
        -fsanitize=pointer-overflow
        -fsanitize=builtin)
    add_compile_options(${ASAN_FLAGS})
    add_link_options(${ASAN_FLAGS})
endif()

if(SANITIZER_THREAD)
    if(SANITIZER)
        message(FATAL_ERROR "SANITIZER_THREAD cannot co exists with SANITIZER flag!")
    endif()
    set(ASAN_FLAGS -fsanitize=thread) 
    add_compile_options(${ASAN_FLAGS})
    add_link_options(${ASAN_FLAGS})
endif()


add_subdirectory(gempyrelib)

option(HAS_AFFILIATES "Affiliates are built" ${USE_DEFAULT_SUBPROJECTS})

option(HAS_TEST "Test are built" ${USE_DEFAULT_SUBPROJECTS})

option(HAS_EXAMPLES "Examples are built" ${USE_DEFAULT_SUBPROJECTS})

option(USE_PYTHON_UI, "Use Python Webview as default UI instead of system browser" ${USE_DEFAULT_PYTHON})
 

if(HAS_AFFILIATES)
    add_subdirectory(affiliates)
endif()

if(HAS_TEST)
    add_subdirectory(test)
endif()

if(HAS_EXAMPLES)
    add_subdirectory(examples)
endif()

