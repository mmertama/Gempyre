
set(GEMPYRE_FUNCTION_DIR ${CMAKE_CURRENT_LIST_DIR})
cmake_minimum_required(VERSION 3.20 FATAL_ERROR)
function (gempyre_add_resources)
    cmake_parse_arguments(
        ADD_RESOURCE
        "MINIFY" "TARGET;PROJECT" "SOURCES"
        ${ARGN}
        )

    if({ADD_RESOUCE_MINIFY})
        set(MINIFY "--minify")
    else()
        set(MINIFY "")
    endif()
	
    if(NOT WIN32)
        find_program(PYTHON "python3" REQUIRED)
        set (PYTHON3 python3)
    else()
        find_program(PYTHON "python" REQUIRED)
        set (PYTHON3 python)
    endif()

    set(PYTHON3 ${PYTHON3} PARENT_SCOPE)

    find_file(GEN_RESOURCE
        genStringResource.py
        PATHS "${GEMPYRE_FUNCTION_DIR}" "${CMAKE_SOURCE_DIR}/scripts" "${CMAKE_CURRENT_LIST_DIR}"
        )

    if(NOT ${GEN_RESOURCE})
        set(CMAKE_FIND_ROOT_PATH "/") # EMSCIPTEN HACK
        find_file(GEN_RESOURCE
            genStringResource.py
            PATHS "${GEMPYRE_FUNCTION_DIR}" "${CMAKE_SOURCE_DIR}/scripts" "${CMAKE_CURRENT_LIST_DIR}"
            REQUIRED
            )
    endif()    

    if (NOT DEFINED ADD_RESOURCE_PATH)
        set(ADD_RESOURCE_PATH "${CMAKE_CURRENT_BINARY_DIR}")
        set(INCDIR "${ADD_RESOURCE_PATH}/${ADD_RESOURCE_TARGET}")
        cmake_path(REMOVE_FILENAME INCDIR)
        target_include_directories(${ADD_RESOURCE_PROJECT} PRIVATE ${INCDIR})
    endif()    
    
    set(TARGET_FULL "${ADD_RESOURCE_PATH}/${ADD_RESOURCE_TARGET}")

    add_custom_command(
            OUTPUT ${TARGET_FULL}
            COMMAND ${PYTHON3} ${GEN_RESOURCE} ${MINIFY} ${TARGET_FULL} ${ADD_RESOURCE_SOURCES}
            DEPENDS ${ADD_RESOURCE_SOURCES}
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            )
   
    add_custom_target(${ADD_RESOURCE_PROJECT}_resource DEPENDS ${TARGET_FULL} ${ADD_RESOURCE_SOURCES})

    add_dependencies(${ADD_RESOURCE_PROJECT} ${ADD_RESOURCE_PROJECT}_resource)
    

    
endfunction()

function (supress_errors)
    cmake_parse_arguments( SUPRESS "FILES" "FLAGS" ${ARGN})
    foreach( U_P ${SUPRESS_FILES})
        if (NOT EXISTS ${U_P})
            message("Cannot suppress ${U_P}")
        endif()
        set_source_files_properties($U_P PROPERTIES COMPILE_OPTIONS ${FLAGS})
    endforeach()  
endfunction()