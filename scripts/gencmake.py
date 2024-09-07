import sys

template = """
cmake_minimum_required (VERSION 3.14)
set(NAME !NAME)
project (${NAME} VERSION 0.1)
set(CMAKE_CXX_STANDARD 20)


include_directories(
	!INCLUDES
)


FILE(GLOB_RECURSE SOCKETS_SRC_CXX "!SOURCES/*.cpp")
FILE(GLOB_RECURSE SOCKETS_SRC_C "!SOURCES/*.c") 
FILE(GLOB_RECURSE SOCKETS_HRD "!HEADERS/*.h") 

add_library(${PROJECT_NAME}
	${SOCKETS_SRC_CXX}
	${SOCKETS_SRC_C}
	${SOCKETS_HRD}
)

"""

pos = 2
while pos < len(sys.argv):
	template = template.replace('!' + sys.argv[pos], sys.argv[pos + 1])
	pos += 2

with open(sys.argv[1], 'w') as f:
	f.write(template)

	
