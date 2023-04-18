#!/bin/bash
set -e
rm -rf $1/test/install_test
mkdir -p  $1/test/install_test
pushd $1/test/install_test

TARGET=$2

echo "extra: $EXTRA_FLAGS"

if [[ ! "$TARGET" == "RELEASE" ]]; then

    cmake ../../../test/install_test -DCMAKE_BUILD_TYPE=Debug $EXTRA_FLAGS
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

    cmake ../../../test/install_test -DCMAKE_BUILD_TYPE=Release $EXTRA_FLAGS
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

if [[ $DISPLAY ]]; then 
    $1/test/install_test/Hello
else
    echo "WARNING: DISPLAY is not set -> verification is not completed!"    
fi

