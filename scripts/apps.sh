#!/bin/bash

set -e

APPS=(  "Tilze"
        "hexview-Gempyre"
        "treeview-Gempyre"
        "calc-Gempyre"
        "mandelbrot-Gempyre"
        )

pushd `dirname "$0"`
pushd ../..

BRANCH=${1:-main}

for i in ${APPS[@]};
    do
        if [[ ! -d $i ]]; then
            git clone https://github.com/mmertama/$i.git
        fi
        pushd $i

        git checkout $BRANCH
        git pull --ff-only

        mkdir -p build
        pushd build

        cmake .. -DCMAKE_BUILD_TYPE=Release
        cmake --build .

        popd
        popd
    done

popd
