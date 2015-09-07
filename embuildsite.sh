#!/bin/bash

# generate versioned site
rm -rf src/drivers/em/deploy
src/drivers/em/scripts/buildsite.py src/drivers/em/site src/drivers/em/deploy

# copy/optimize shaders
#srcdir=../em-fceux/src/drivers/em/assets/shaders
#dstdir=../em-fceux/src/drivers/em/assets/data
#if 
#for f in $srcdir/*.vert
#do
#./glslopt -v -2 $f $dstdir/${f##*/}
#done
#for f in $srcdir/*.frag
#do
#./glslopt -f -2 $f $dstdir/${f##*/}
#done
