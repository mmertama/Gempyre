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
    set(CMAKE_CXX_FLAGS "-DWINDOWS_OS ${CMAKE_CXX_FLAGS}")
    add_compile_options(-D_WIN32)
    add_compile_options(-DWINDOWS_OS)
    add_compile_options(-DLIBUS_NO_SSL)
    add_compile_options(-DNOMINMAX)
    set(CMAKE_DEBUG_POSTFIX d)
else()
    set(CMAKE_CXX_FLAGS "-Wall -Wextra ${CMAKE_CXX_FLAGS}")
endif()

if(APPLE)
    set(CMAKE_CXX_FLAGS "-DMAC_OS ${CMAKE_CXX_FLAGS}")
elseif(RASPBERRY)
    set(CMAKE_CXX_FLAGS "-DUNIX_OS -DRASPBERRY_OS -Wno-psabi ${CMAKE_CXX_FLAGS")  # Should be Rasbian?
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
    add_compile_options(-fPIC)
elseif(ANDROID)
     set(CMAKE_CXX_FLAGS "-DANDROID_OS ${CMAKE_CXX_FLAGS}")
elseif(UNIX)
   set(CMAKE_CXX_FLAGS "-DUNIX_OS ${CMAKE_CXX_FLAGS}")
   set(CMAKE_POSITION_INDEPENDENT_CODE ON)
   add_compile_options(-fPIC)
endif()

option(CI_ACTIONS "Github Actions build" OFF)

if(CI_ACTIONS)
    add_compile_options(-DCI_ACTIONS)
    message(WARNING "This is for Github actions")
endif()

add_subdirectory(gempyrelib)

option(HAS_AFFILIATES "Affiliats are built" ON)
if(MINGW)
    message(WARNING "Tests not working ok with MINGW")
    option(HAS_TEST "Test are built" OFF)
else()
    option(HAS_TEST "Test are built" ON)
endif()

option(HAS_EXAMPLES "Examples are built" ON)

if(HAS_AFFILIATES)
    add_subdirectory(affiliates)
endif()

if(HAS_TEST)
    add_subdirectory(test)
endif()

if(HAS_EXAMPLES)
    add_subdirectory(examples)
endif()


