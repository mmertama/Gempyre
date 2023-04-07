cmake_minimum_required (VERSION 3.18)
project(gempyre LANGUAGES CXX)

# enable ctests, starting from the top build folder
include(CTest)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
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
else()
    add_compile_options(-Wall -Wextra)
endif()

if(APPLE)
    add_compile_definitions(MAC_OS)
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

option(CI_ACTIONS "Github Actions build" OFF)

if(CI_ACTIONS)
    add_compile_options(-DCI_ACTIONS)
    message(WARNING "This is for Github actions")
endif()

add_subdirectory(gempyrelib)

option(HAS_AFFILIATES "Affiliates are built" ON)

option(HAS_TEST "Test are built" ON)

option(HAS_EXAMPLES "Examples are built" ON)

option(USE_PYTHON_UI, "Use Python Webview as default UI instead of system browser" OFF)
  

if(HAS_AFFILIATES)
    add_subdirectory(affiliates)
endif()

if(HAS_TEST)
    add_subdirectory(test)
endif()

if(HAS_EXAMPLES)
    add_subdirectory(examples)
endif()

