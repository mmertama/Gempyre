#!/bin/bash

set -euo pipefail

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
        echo "Build $i"
        if [[ ! -d $i ]]; then
            git clone https://github.com/mmertama/$i.git
        fi

        pushd $i

        O_BRANCH=$(git branch --show-current)
        if [[ $BRANCH != $O_BRANCH ]]; then
            git checkout $BRANCH
        fi

        O_BRANCH=$(git branch --show-current)
        if [[ $BRANCH != $O_BRANCH ]]; then
            echo "Branch $BRANCH not found"
            exit
        fi

        set +e
        R_BRANCH=$(git branch -r | grep /${BRANCH}$)
        if [[ ! -z "$R_BRANCH" ]]; then
            echo "Pull $R_BRANCH"
            git pull --ff-only
        fi
        set -e

        mkdir -p build
        pushd build

        cmake .. -DCMAKE_BUILD_TYPE=Releases || true 2>&1
        cmake --build . || true

        echo "$i done"

        popd
        popd
    done

popd
