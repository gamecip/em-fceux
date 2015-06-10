#!/bin/bash
# Huh?? Why we suddenly start needing pwd here?
pushd ~/emsdk && pwd && source emsdk_env.sh && popd
echo "const char *gitversion = \"commit `git rev-parse --short HEAD`\";" > src/drivers/em/gitversion.c 
emscons scons -j 4 $@
input_in=src/drivers/em/em-input.inc.h
input_js=src/drivers/em/deployment/input.js
echo "// WARNING! AUTOMATICALLY GENERATED FILE. DO NOT EDIT." > $input_js
#emcc -D 'KEYS_PRE=' -D 'KEYS_POST=' -D 'KEY(i_,d_,e_,t_)=var FCEM_ ## e_ = i_;' -E $input_in -P >> $input_js 
emcc -D 'KEYS_PRE=FCEM.inputs = {' -D 'KEYS_POST=};' -D 'KEY(i_,d_,e_,t_)=i_ : [ d_, t_ ],' -E $input_in -P >> $input_js 
