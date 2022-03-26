@echo on
if "%VSCMD_ARG_HOST_ARCH%"=="x64" goto pass_ver
echo Execute in the x64 Native tools command prompt.
echo "msvc_install.bat <DIR>"
echo DIR is optional and points to the install dir, defaults defined in GnuInstallDirs where the find_package should find it.
echo Warning: if a DIR is a relative directory, it refers to msvc_build folder. 
goto exit
:pass_ver

SET ONOFF=OFF
if "%1"=="-all" set ONOFF=ON
if "%2"=="-all" set ONOFF=ON

set PREFIX=
if "%1"=="" goto no_pre1
    if NOT "%1"=="-all" set PREFIX=-DCMAKE_INSTALL_PREFIX=%1
:no_pre1
if "%2"=="" goto no_pre2
    if NOT "%2"=="-all" set PREFIX=-DCMAKE_INSTALL_PREFIX=%2
:no_pre2

echo Build all is %ONOFF%


if not exist "msvc_build" mkdir msvc_build

pushd msvc_build
if exist "install.log" del install.log
cmake .. -DCMAKE_BUILD_TYPE=Debug -DHAS_AFFILIATES=%ONOFF% -DHAS_TEST=%ONOFF% -DHAS_EXAMPLES=%ONOFF% %PREFIX%
if %ERRORLEVEL% NEQ 0 popd && exit /b %ERRORLEVEL%
cmake --build . --config Debug
if %ERRORLEVEL% NEQ 0 popd && exit /b %ERRORLEVEL%
set BUILD_PATH=%CD%
popd
echo Start an elevated prompt for an install.
powershell -Command "Start-Process scripts\win_inst.bat -Verb RunAs -ArgumentList "%BUILD_PATH%,Debug"

pushd msvc_build
cmake .. -DCMAKE_BUILD_TYPE=Release -DHAS_AFFILIATES=%ONOFF% -DHAS_TEST=%ONOFF% -DHAS_EXAMPLES=%ONOFF% %PREFIX%
cmake --build . --config Release
set BUILD_PATH=%CD%
popd
echo Start an elevated prompt for an install.
powershell -Command "Start-Process scripts\win_inst.bat -Verb RunAs -ArgumentList %BUILD_PATH%"

echo done
:exit
