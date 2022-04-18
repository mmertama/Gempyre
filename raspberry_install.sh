set -e 

CMD_STR='$ ./raspberry_install.sh [-all] [-dir <DIR>] [-debug] [-release]'

ONOFF="OFF"
PREFIX=""
TARGET=""

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


mkdir -p build

pushd build

if [[ ! "$TARGET" == "RELEASE" ]]; then
    cmake .. -DRASPBERRY=1 -DCMAKE_BUILD_TYPE=Debug -DHAS_AFFILIATES=OFF -DHAS_TEST=$ONOFF -DHAS_EXAMPLES=$ONOFF $PREFIX
    cmake --build . --config Debug
# if cmake is installed using snap, you may need to crete its symbolic link to /usr/local/bin
# otherwise sudo may not find it
    sudo cmake --install . --config Debug
fi    

if [[ ! "$TARGET" == "DEBUG" ]]; then
    cmake .. -DRASPBERRY=1 -DCMAKE_BUILD_TYPE=Release -DHAS_AFFILIATES=OFF -DHAS_TEST=$ONOFF -DHAS_EXAMPLES=$ONOFF $PREFIX
    cmake --build . --config Release
    sudo cmake --install . --config Release
fi

popd


echo "Run install test"
export EXTRA_FLAGS="-DRASPBERRY=1";test/install_test/install_test.sh build
