#!/bin/bash
set -e 

CMD_STR='$ ./mingw_install.sh [-all] [-dir <DIR>] [-debug] [-release] [-ninja]'

export PATH="/mingw64/bin/:$PATH" 

if ! [ -x "$(command -v gcc)" ]; then
    echo gcc not found, are you using msys terminal?
    echo see $ pacman -S mingw-w64-x86_64-toolchain
    echo $CMD_STR
    echo DIR is optional and points to the install dir, defaults defined in GnuInstallDirs where the find_package should find it.
    exit 1
fi  


if ! [ -x "$(command -v git)" ]; then
    echo git not found, do you have git installed for your msys terminal?
    echo $CMD_STR
    echo DIR is optional and points to the install dir, defaults defined in GnuInstallDirs where the find_package should find it.
    exit 1
fi  

if ! [ -x "$(command -v ninja)" ]; then
    echo ninja not found
     echo $CMD_STR
    echo DIR is optional and points to the install dir, defaults defined in GnuInstallDirs where the find_package should find it.
    exit 1
 fi

 if ! [ -x ${CMAKE} ]; then
    echo ${CMAKE} not found
    echo $CMD_STR
    echo DIR is optional and points to the install dir, defaults defined in GnuInstallDirs where the find_package should find it.
    exit 1
 fi 

ONOFF="OFF"
PREFIX=""
TARGET=""
BUILD_TOOL="MinGW Makefiles"

while [ -n "$1" ]; do # while loop starts

    if [ "$1" == '-all' ]; then
        ONOFF="ON"
    fi
    if [ "$1" == '-dir' ]; then
         PREFIX="$2"
         shift   
    fi

    if [ "$1" == '-debug' ]; then
         TARGET="DEBUG"
    fi

    if [ "$1" == '-release' ]; then
         TARGET="RELEASE"
    fi

     if [ "$1" == '-ninja' ]; then
         BUILD_TOOL="Ninja"
    fi

	shift
done


if [[ $PREFIX -ne "" && ! -d $PREFIX ]]; then
    echo "$PREFIX not found"
    return
fi

if [[ $PREFIX -ne "" && -d $PREFIX ]]; then
    PREFIX=-DCMAKE_INSTALL_PREFIX=$PREFIX
    return
fi


mkdir -p mingw_build

pushd mingw_build

if [ -f  "install.log" ]; then 
    rm install.log
fi

PWD=$(pwd)
BUILD_PATH=$(cygpath -w $PWD)

if [[ ! "$TARGET" == "RELEASE" ]]; then

    cmake .. -G ${BUILD_TOOL} -DCMAKE_BUILD_TYPE=Debug -DHAS_AFFILIATES=$ONOFF -DHAS_TEST=$ONOFF -DHAS_EXAMPLES=$ONOFF $PREFIX
    cmake --build . --config Debug

    popd

    echo Start an elevated a prompt for a debug install.
    powershell -Command "Start-Process scripts\win_inst.bat -Verb RunAs -ArgumentList \"$BUILD_PATH\",Debug"

    pushd mingw_build

fi

if [[ ! "$TARGET" == "DEBUG" ]]; then

    cmake ..  -G  ${BUILD_TOOL} -DCMAKE_BUILD_TYPE=Release -DHAS_AFFILIATES=$ONOFF -DHAS_TEST=$ONOFF -DHAS_EXAMPLES=$ONOFF $PREFIX
    cmake --build . --config Release

    popd
    echo Start an elevated a prompt for a release install.
    powershell -Command "Start-Process scripts\win_inst.bat -Verb RunAs -ArgumentList \"$BUILD_PATH\",Release"
fi 

echo "Run install test"
export EXTRA_FLAGS="-G $BUILD_TOOL";test/install_test/install_test.sh mingw_build $TARGET

echo done
