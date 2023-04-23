rmdir /s /q "msvc_build\test\install_test"
mkdir  "msvc_build\test\install_test"
pushd "msvc_build\test\install_test"
cmake ..\..\..\test\install_test -DCMAKE_BUILD_TYPE=Debug
if %ERRORLEVEL% EQU 0 goto installa_ok
popd
echo "Verify install failed! (cmake debug) Abort here"
exit /b %ERRORLEVEL%
:installa_ok
cmake --build . --config Debug
if %ERRORLEVEL% EQU 0 goto installb_ok
echo "Verify install failed!(build debug)  Abort here"
popd
exit /b %ERRORLEVEL%
:installb_ok
popd

rmdir /s /q  "msvc_build\test\install_test"
mkdir  "msvc_build\test\install_test"
pushd "msvc_build\test\install_test"
cmake ..\..\..\test\install_test -DCMAKE_BUILD_TYPE=Release
if %ERRORLEVEL% EQU 0 goto installc_ok
popd
echo "Verify install failed!(cmake release)  Abort here"
exit /b %ERRORLEVEL%
:installc_ok
cmake --build . --config Release
if %ERRORLEVEL% EQU 0 goto installd_ok
echo "Verify install failed! (build release) Abort here"
popd
exit /b %ERRORLEVEL%
:installd_ok
popd

if "!GITLAB_CI!"=="" msvc_build\test\install_test\Release\Hello.Exe 
