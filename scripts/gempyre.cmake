set(GEMPYRE_FUNCTION_DIR ${CMAKE_CURRENT_LIST_DIR})
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

    find_file(GEN_RESOURCE
        genStringResource.py
        PATHS "${GEMPYRE_FUNCTION_DIR}" "${CMAKE_SOURCE_DIR}/scripts" "${CMAKE_CURRENT_LIST_DIR}"
        REQUIRED
        )
      
    add_custom_command(
        OUTPUT ${ADD_RESOURCE_TARGET}
        COMMAND ${PYTHON3} ${GEN_RESOURCE} ${MINIFY} ${ADD_RESOURCE_TARGET} ${ADD_RESOURCE_SOURCES}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        )
    
    add_custom_target(${ADD_RESOURCE_PROJECT}_resource DEPENDS ${ADD_RESOURCE_TARGET} ${ADD_RESOURCE_SOURCES})
    add_dependencies(${ADD_RESOURCE_PROJECT} ${ADD_RESOURCE_PROJECT}_resource)
    
endfunction()
