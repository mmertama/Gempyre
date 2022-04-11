ONOFF="OFF"
TARGET=""

while [ -n "$1" ]; do # while loop starts

   if [ "$1" == '-all' ]; then
       ONOFF="ON"
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

mkdir -p build

pushd build

if [[ ! "$TARGET" == "RELEASE" ]]; then
    cmake ..  -DCMAKE_BUILD_TYPE=Debug -DHAS_AFFILIATES=$ONOFF -DHAS_TEST=$ONOFF -DHAS_EXAMPLES=$ONOFF
    cmake --build . --config Debug
    sudo cmake --install . --config Debug
fi

if [[ ! "$TARGET" == "DEBUG" ]]; then
cmake ..  -DCMAKE_BUILD_TYPE=Release -DHAS_AFFILIATES=$ONOFF -DHAS_TEST=$ONOFF -DHAS_EXAMPLES=$ONOFF
cmake --build . --config Release
sudo cmake --install . --config Release
fi

popd

echo "Run install test"
test/install_test/install_test.sh build
