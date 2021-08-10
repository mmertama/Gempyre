cmake_minimum_required (VERSION 3.18)
set(NAME gempyrepyclient)
project (${NAME})

find_package(Python3 COMPONENTS Interpreter REQUIRED)

source_group(FILES
   pyclient.py
   )

add_custom_target(
    ${NAME} ALL
    DEPENDS ${PROJECT_SOURCE_DIR}/pyclient.py
    COMMAND ${CMAKE_COMMAND} -E copy
            ${PROJECT_SOURCE_DIR}/pyclient.py
            ${CMAKE_CURRENT_BINARY_DIR}/pyclient.py)


if(Python3_FOUND)
#    set(CONFIG_PY_DIR ${PROJECT_SOURCE_DIR})
#    configure_file(pyclient.py.in ${PROJECT_BINARY_DIR}/pyclient.py @ONLY)
#    set(CONFIG_PY_DIR ${PYTHON_INSTALL_DIR})
#    configure_file(pyclient.py.in ${PROJECT_BINARY_DIR}/installFiles/pyclient.py @ONLY)
#    install(PROGRAMS ${PROJECT_BINARY_DIR}/installFiles/pyclient.py DESTINATION bin)
else()
    message("Python3 not found")
endif()
