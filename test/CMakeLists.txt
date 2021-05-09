include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
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

add_subdirectory(apitests)

