PREFIX=$1

if [[ $PREFIX -ne "" && ! -d $PREFIX ]]; then
    echo "$PREFIX not found"
    return 
fi

mkdir -p build

pushd build

# tests wont link on rasberry unless chromium get compiled on c++17 (todo)
cmake .. -DRASPBERRY=1 -DCMAKE_BUILD_TYPE=Debug -DHAS_AFFILIATES=OFF -DHAS_TEST=OFF -DHAS_EXAMPLES=OFF
cmake --build . --config Debug
# if cmake is installed using snap, you may need to crete its symbolic link to /usr/local/bin
# otherwise sudo may not find it
sudo cmake --install . --config Debug

cmake .. -DRASPBERRY=1 -DCMAKE_BUILD_TYPE=Release -DHAS_AFFILIATES=OFF -DHAS_TEST=OFF
cmake --build . --config Release
sudo cmake --install . --config Release

popd
