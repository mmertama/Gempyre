if(NOT ANDROID)
    add_subdirectory(py_uisrv)
    add_subdirectory(test_rpc)
else()
    message("Android does not support affiliates")
endif()
