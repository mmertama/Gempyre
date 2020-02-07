cmake_minimum_required (VERSION 3.8)

project (helloworld)

set(CMAKE_CXX_STANDARD 17)

include_directories(
     ../../telexlib/include
)

find_package (telex REQUIRED PATHS ../..)

add_executable(helloworld main.cpp)

target_link_libraries (helloworld telex)



