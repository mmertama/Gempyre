PREFIX=$1

if [[ $PREFIX -ne "" && ! -d $PREFIX ]]; then
    echo "$PREFIX not found"
    return 
fi

mkdir -p build

pushd build

# only for C++, for DCMAKE_C_COMPILER is leaved default
CXXCOMPILER=/opt/gcc-9.1.0/bin/c++-9.1
CCOMPILER=/usr/bin/gcc

#cmake ..  -DCMAKE_CXX_COMPILER=$CXXCOMPILER -DCMAKE_C_COMPILER=$CCOMPILER -DCMAKE_BUILD_TYPE=Debug -DHAS_AFFILIATES=OFF -DHAS_TEST=OFF -DHAS_EXAMPLES=OFF
#cmake --build . --config Debug
#sudo cmake --install . --config Debug

#cmake .. -DCMAKE_CXX_COMPILER=$CXXCOMPILER -DCMAKE_C_COMPILER=$CCOMPILER -DCMAKE_BUILD_TYPE=Release -DHAS_AFFILIATES=OFF -DHAS_TEST=OFF -DHAS_EXAMPLES=OFF
#cmake --build . --config Release
#sudo cmake --install . --config Release

# tests wont link on rasberry unless chromium get compiled on c++17 (maybe)
cmake .. -DRASPBERRY=1 -DCMAKE_CXX_COMPILER=$CXXCOMPILER -DCMAKE_C_COMPILER=$CCOMPILER -DCMAKE_BUILD_TYPE=Debug -DHAS_AFFILIATES=OFF -DHAS_TEST=OFF
cmake --build . --config Debug
# if cmake is installed using snap, you may need to crete its symbolic link to /usr/local/bin
# otherwise sudo may not find it
##sudo cmake --install . --config Debug

##cmake .. -DCMAKE_CXX_COMPILER=$CXXCOMPILER -DCMAKE_C_COMPILER=$CCOMPILER -DCMAKE_BUILD_TYPE=Release
##cmake --build . --config Release
##sudo cmake --install . --config Release

popd
