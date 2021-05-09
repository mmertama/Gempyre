#!/bin/bash
find . -name ".git" | while read gitname;
 do 
	dirname=$(dirname $gitname);
	pushd $dirname > /dev/null
	echo $(basename $dirname)
	echo $(git describe --tags)
	popd > /dev/null
 done

