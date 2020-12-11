function (addResource)
    cmake_parse_arguments(
        ADD_RESOURCE
        "MINIFY" "TARGET;PROJECT" "SOURCES"
        ${ARGN}
        )
    
    if(NOT EXISTS ${GEMPYRE_DIR})
        if(EXISTS $ENV{GEMPYRE_DIR})
            set(GEMPYRE_DIR $ENV{GEMPYRE_DIR})
        elseif(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/gempyrelib)
            set(GEMPYRE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
        elseif(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/../gempyrelib)
               set(GEMPYRE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/..)
        elseif(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/../Gempyre/gempyrelib)
               set(GEMPYRE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../Gempyre)
	else()
            message(FATAL_ERROR "GEMPYRE_DIR is not set neither GEMPYRE_DIR Environment variable is not pointing to Gempyre dir (current working dir: ${CMAKE_CURRENT_SOURCE_DIR})")
        endif()
    endif()
    
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
      
    add_custom_command(
        OUTPUT ${ADD_RESOURCE_TARGET}
        COMMAND ${PYTHON3} ${GEMPYRE_DIR}/scripts/genStringResource.py ${MINIFY} ${ADD_RESOURCE_TARGET} ${ADD_RESOURCE_SOURCES}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        )
    
    add_custom_target(${ADD_RESOURCE_PROJECT}_resource DEPENDS ${ADD_RESOURCE_TARGET} ${ADD_RESOURCE_SOURCES})
    add_dependencies(${ADD_RESOURCE_PROJECT} ${ADD_RESOURCE_PROJECT}_resource)
    
endfunction()
