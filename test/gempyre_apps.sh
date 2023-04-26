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
mkdif $TMP
pushd $TMP       

for i in ${APPS[@]};
    do
        echo "Build $i"
        if [[ ! -d $i ]]; then
            git clone https://github.com/mmertama/$i.git
        fi

        pushd $i

	set +e
      	git checkout main
      	git checkout master
      	set -e 
      	
      	git pull
        
        mkdir -p build
        pushd build
	rm CMakeCache.txt
        rm -rf CMakeFiles
        cmake .. -DCMAKE_BUILD_TYPE=Release || true 2>&1
        cmake --build . || true

        echo "$i done"

        popd
        popd
    done

popd
popd       
