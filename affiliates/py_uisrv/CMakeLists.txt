cmake_minimum_required (VERSION 3.18)
project (gepmpyreuisrv)

find_package(Python3 COMPONENTS Interpreter REQUIRED)

source_group(PYSOURCE ui_srv.py ui_srvclient.py)

add_custom_target(
    gepmpyreuisrv ALL
    DEPENDS ${PROJECT_SOURCE_DIR}/ui_srvclient.py
    COMMAND ${CMAKE_COMMAND} -E copy
            ${PROJECT_SOURCE_DIR}/ui_srvclient.py
            ${CMAKE_CURRENT_BINARY_DIR}/ui_srvclient.py)


