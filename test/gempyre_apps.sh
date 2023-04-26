#!/bin/bash

set -euo pipefail

TMP=/tmp/gempyre_apps

APPS=(  "calc-Gempyre"
	    "Tilze"
        "mandelbrot-Gempyre"
        "hexview-Gempyre"
        "treeview-Gempyre"
        "Gempyre-Python"  
       )

rm -rf $TMP
mkdir $TMP
pushd $TMP       

for i in ${APPS[@]};
    do
        echo "Build $i"
        if [[ ! -d $i ]]; then
            git clone https://github.com/mmertama/$i.git
        fi

        pushd $i

        if git branch | grep "main"; then 
      	    git checkout main
        fi

      	if git branch | grep "master"; then 
      	    git checkout master
        fi
      	
      	git pull
        
        mkdir -p build
        pushd build
	    rm -f CMakeCache.txt
        rm -rf CMakeFiles

        cmake .. -DCMAKE_BUILD_TYPE=Release
        if [ $? -ne 0 ]; then
            echo CMake config on $i has errors!
            exit -1
        fi          
        
        cmake --build . 
        if [ $? -ne 0 ]; then
            echo CMake build on $i has errors!
            exit -1
        fi    

        echo "$i done"

        popd
        popd
    done

popd


rm -rf $TMP
