#!/bin/bash
set -e
rm -rf $1/test/install_test
mkdir -p  $1/test/install_test
pushd $1/test/install_test

TARGET=$2

if [[ ! "$TARGET" == "RELEASE" ]]; then

    cmake ../../../test/install_test -DCMAKE_BUILD_TYPE=Debug
    err=$?
    if [ ! $err -eq 0 ]; then
        popd
        echo "Verify install failed:${err}! (cmake debug) Abort here"
        exit $err
    fi    
    cmake --build . --config Debug
    err=$?
    if [ ! $err -eq 0 ]; then
        echo "Verify install failed:${err}! (build debug)  Abort here"
        popd
        exit $err
    fi    
fi

popd

if [[ ! "$TARGET" == "DEBUG" ]]; then
    rm -rf $1/test/install_test
    mkdir -p  $1/test/install_test
fi    

pushd $1/test/install_test

if [[ ! "$TARGET" == "DEBUG" ]]; then

    cmake ../../../test/install_test -DCMAKE_BUILD_TYPE=Release
    err=$?
    if [ ! $err -eq 0 ]; then
        popd
        echo "Verify install failed:${err}! (cmake release) Abort here"
        exit $err
    fi 
    cmake --build . --config Release
    err=$?
    if [ ! $err -eq 0 ]; then
        echo "Verify install failed:${err}! (build release) Abort here"
        popd
        exit $err
    fi

fi

popd

 $1/test/install_test/Hello
