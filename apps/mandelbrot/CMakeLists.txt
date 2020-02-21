cmake_minimum_required (VERSION 3.8)

set(NAME mandelbrot)
project (${NAME})

include(GNUInstallDirs)
include(ExternalProject)

set(CMAKE_CXX_STANDARD 17)

if(MSVC)
    set(USE_APML TRUE)
else()
    set(USE_APML FALSE)
    find_path(gmp gmp.h)
    if(gmp)
        include_directories(${gmp})
        set(USE_MPFR TRUE)
    else()
        if(APPLE)
             message("Consider: $brew install gmp")
         else()
            message("Consider: $sudo apt install libgmp-dev")
        endif()
    endif()
endif()

if(USE_APML)
    add_compile_options("-DUSE_APML")
    set(BM_PATH src/Precision)
    set(BM_INCLUDE_PATH ${BM_PATH})
    file(GLOB BM_SRC "${BM_PATH}/*.cpp" "${BM_PATH}/*.h")
    if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        message("Supress annoyances...")
        add_compile_options("-Waddress-of-temporary")
        set_source_files_properties(${BM_SRC} PROPERTIES COMPILE_FLAGS "-Wno-everything")
    endif()
endif()

if(USE_MPFR)
     add_compile_options("-DUSE_MPFR")
     set(BM_PATH src/mpfr)
     set(BM_INCLUDE_PATH ${BM_PATH}/src)
   #  find_library(BM_LIB mpfr PATHS  ${BM_PATH} ${BM_PATH}/src PATH_SUFFIXES .libs)
endif()

find_package (telex REQUIRED PATHS ../..)

include_directories(
     ../../telexlib/include
    include
    ${BM_INCLUDE_PATH}
)


add_executable(${PROJECT_NAME}
    src/mandelbrot.h
    src/mandelbrotdraw.h
    src/main.cpp    
    gui/${NAME}.html
    gui/${NAME}.css
    gui/${NAME}.png
    ${BM_SRC}
    )


if(USE_MPFR)
    externalproject_add(mpfr
        BUILD_IN_SOURCE 1
        PREFIX ${CMAKE_CURRENT_SOURCE_DIR}/${mpfr}
        URL https://www.mpfr.org/mpfr-current/mpfr-4.0.2.tar.gz
        CONFIGURE_COMMAND "./configure"
        INSTALL_COMMAND ""
        )
    add_dependencies(${PROJECT_NAME} mpfr)
    set(BM_LIB "${BM_PATH}/src/.libs/${CMAKE_STATIC_MODULE_PREFIX}mpfr${CMAKE_STATIC_LIBRARY_SUFFIX}")
endif()

addResource(PROJECT ${PROJECT_NAME} TARGET include/${NAME}_resource.h SOURCES gui/${NAME}.html gui/${NAME}.css gui/${NAME}.png)

target_link_libraries (${PROJECT_NAME} telex ${BM_LIB})
 

