PREFIX=$1

if [[ $PREFIX -ne "" && ! -d $PREFIX ]]; then
    echo "$PREFIX not found"
    return 
fi

mkdir -p build

pushd build

cmake ..  -DCMAKE_BUILD_TYPE=Debug -DHAS_AFFILIATES=OFF -DHAS_TEST=OFF -DHAS_EXAMPLES=OFF
cmake --build . --config Debug
sudo cmake --install . --config Debug

cmake ..  -DCMAKE_BUILD_TYPE=Release -DHAS_AFFILIATES=OFF -DHAS_TEST=OFF -DHAS_EXAMPLES=OFF
cmake --build . --config Release
sudo cmake --install . --config Release

popd
