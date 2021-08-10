cmake_minimum_required (VERSION 3.18)
set(NAME affiliates_include)
project (${NAME})
add_library(${NAME} INTERFACE)
target_include_directories(${NAME} INTERFACE include/)
target_sources(${NAME} INTERFACE
    gempyre_client.h
    )



