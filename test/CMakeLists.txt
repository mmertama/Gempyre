include(FetchContent)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/scripts")

if(RASPBERRY)
set(CMAKE_CXX_FLAGS "-D_POSIX_PATH_MAX=256 ${CMAKE_CXX_FLAGS}")
endif()


FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG main
)

# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_GetProperties(googletest)

set(INSTALL_GMOCK OFF CACHE BOOL "" FORCE)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

set_tests_properties(${Tests} PROPERTIES TIMEOUT 10)

set(TEST_INCLUDE_DIR ${googletest_SOURCE_DIR}/googletest/include())
set(TEST_LIBRARY_DIR ${googletest_BINARY_DIR})


if(APPLE)
  message(WARNING "Disable mock browser and its test on OSX as Clang 14.0 fails with libwebsocket")
elseif(WIN32)
  message(WARNING "Mock browser stopped working in msvc - connection error 10035 - todo")    
else()
  set(HAS_MOCK 1)
  add_compile_definitions(HAS_MOCK)
  add_subdirectory(mock_browser)
endif()


add_subdirectory(apitests)
add_subdirectory(unittests)
if(NOT DEFINED ACTIONS)  # on actions EXCLUDE_FROM_ALL is not proof
  add_subdirectory(install_test EXCLUDE_FROM_ALL)
endif()

