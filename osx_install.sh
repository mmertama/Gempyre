set -e

ONOFF="OFF"
TARGET=""
ALL_TARGETS=false

function build() {
	build_folder=$(echo "$1" | tr '[:upper:]' '[:lower:]')
	mkdir -p $build_folder
	pushd $build_folder
	cmake ../..  -G Ninja -DCMAKE_BUILD_TYPE=$1 -DHAS_AFFILIATES=$ONOFF -DHAS_TEST=$ONOFF -DHAS_EXAMPLES=$ONOFF
        cmake --build . --config $1
        sudo cmake --install . --config $1
	popd
}

while [ -n "$1" ]; do # while loop starts

   if [ "$1" == '-all' ]; then
       ONOFF="ON"
   fi

   if [ "$1" == '-alltargets' ]; then
       ALL_TARGETS=true
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

if [[ ! "$TARGET" == "RELEASE" && "$ALL_TARGETS" == "false" ]]; then
    build "Debug"
fi

if [[ ! "$TARGET" == "DEBUG" && "$ALL_TARGETS" == "false" ]]; then
    build "Release"	
fi

if [[ "$ALL_TARGETS" == "true" ]]; then
    build_types=("Debug" "Release" "RelWithDebInfo" "MinSizeRel")
    for build_type in "${build_types[@]}"; do
        build $build_type
    done
fi

popd

echo "Run install test"
bash test/install_test/install_test.sh build
