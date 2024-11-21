rem @echo off
echo This file is run in elevated mode due it will install libaries into system.
echo The location is documented along GnuInstallDirs.
set PARAM="--config Release"
if "%2"=="Debug" set PARAM="--config Debug"
cd "%~1" || (echo Failed to change dir to %~1 & exit /b 1)
echo "install: %~1 %~2 --> %PARAM%" >> install.log
cmake --install . %PARAM% >> install.log
echo "install done" >> install.log
echo "done %~1/install.log"
