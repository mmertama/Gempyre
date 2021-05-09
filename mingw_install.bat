@echo off
where gcc
IF ERRORLEVEL 00 goto pass1
echo gcc not found
echo "mingw_install.bat <DIR>"
echo DIR is optional and points to the install dir, defaults defined in GnuInstallDirs where the find_package should find it.
goto exit
:pass1
where ninja
IF ERRORLEVEL 00 goto pass2
echo ninja not found
echo "mingw_install.bat <DIR>"
echo DIR is optional and points to the install dir, defaults defined in GnuInstallDirs where the find_package should find it.
goto exit
:pass2

if not "%1"=="" set PREFIX=-DCMAKE_INSTALL_PREFIX=%1
if "%1"=="" set PREFIX=-UCMAKE_INSTALL_PREFIX
set PREFIX=

if not exist "mingw_build" mkdir mingw_build

pushd mingw_build
if exist "install.log" del install.log
cmake ..  -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Debug -DHAS_AFFILIATES=OFF -DHAS_TEST=OFF -DHAS_EXAMPLES=OFF %PREFIX%
cmake --build . --config Debug

set BUILD_PATH=%CD%
popd
echo Start an elevated prompt for an install.
powershell -Command "Start-Process scripts\win_inst.bat -Verb RunAs -ArgumentList "%BUILD_PATH%,Debug"


pushd mingw_build
cmake ..  -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release -DHAS_AFFILIATES=OFF -DHAS_TEST=OFF -DHAS_EXAMPLES=OFF %PREFIX%
cmake --build . --config Release
set BUILD_PATH=%CD%
popd
echo Start an elevated prompt for an install.
powershell -Command "Start-Process scripts\win_inst.bat -Verb RunAs -ArgumentList %BUILD_PATH%"

echo done
:exit
