cmake_minimum_required (VERSION 3.8)
set(NAME telexclient)
project (${NAME})

set(CMAKE_CXX_STANDARD 17)

if(WIN32)
	set (CMAKE_PREFIX_PATH "$ENV{QTDIR}/lib/cmake")
	if(NOT EXISTS ${CMAKE_PREFIX_PATH})
		message(FATAL "Unable to find cmake packages for Qt!")
	endif()
endif()

find_package(Qt5 COMPONENTS Widgets WebEngineWidgets WebSockets REQUIRED)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

add_executable(${PROJECT_NAME}
    mainwindow.ui
    mainwindow.cpp
    main.cpp
    mainwindow.h
)

target_link_libraries(${PROJECT_NAME} Qt5::Widgets Qt5::WebSockets Qt5::WebEngineWidgets)
