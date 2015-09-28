#!/bin/bash
# copy/optimize shaders
glslopt=../glsl-optimizer/glslopt
srcdir=src/drivers/em/assets/shaders
dstdir=src/drivers/em/assets/data
if [ ! -f $glslopt ]
then
echo "GLSL shader optimizer not found at: " $glslopt
echo "Please configure optimizeshader.sh script to point to the glslopt binary."
echo ""
echo "NOTE: optimizeshader.sh script IS NOT mandatory for em-fceux build."
echo "Only run it after modifying the shaders and before calling embuild.sh"
echo "How to build glslopt (at 28-Sep-2015):"
echo ""
echo "  git clone https://github.com/aras-p/glsl-optimizer.git ../glsl-optimizer"
echo "  cd ../glsl-optimizer"
echo "  cmake . && make glslopt"
echo ""
echo "The previous will work in Linux and OS X. It needs at least cmake (and maybe others)."
exit 1
fi

echo "Optimizing vertex shaders:"
for f in $srcdir/*.vert
do
dstf=$dstdir/${f##*/}
echo $f " -> " $dstf
$glslopt -v -2 $f $dstf
done

echo "Optimizing fragment shaders:"
for f in $srcdir/*.frag
do
dstf=$dstdir/${f##*/}
echo $f " -> " $dstf
$glslopt -f -2 $f $dstf
done

