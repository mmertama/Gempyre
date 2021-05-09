rem @echo off
echo This file is run in elevated mode due it will install libaries into system.
echo The location is documented along GnuInstallDirs.
set PARAM=
if "%2"=="Debug" set PARAM=--config Debug
cd %1
cmake --install . %PARAM% >> install.log
