#!/bin/bash
# Huh?? Why we suddenly start needing pwd here?
pushd ~/emsdk && pwd && source emsdk_env.sh && popd
echo "const char *gitversion = \"commit `git rev-parse --short HEAD`\";" > src/drivers/em/gitversion.c 
emscons scons -j 4 $@
config_js=src/drivers/em/deployment/config.js
input_inc=src/drivers/em/em-input.inc.h
config_inc=src/drivers/em/em-config.inc.h
echo "// WARNING! AUTOMATICALLY GENERATED FILE. DO NOT EDIT." > $config_js
emcc -D 'CONTROLLER_PRE=' -D 'CONTROLLER_POST=' -D 'CONTROLLER(i_,e_)=e_ = i_;' -E $config_inc -P >> $config_js 
emcc -D 'INPUT_PRE=FCEM.inputs = {' -D 'INPUT_POST=};' -D 'INPUT(i_,d_,e_,t_)=i_ : [ d_, t_ ],' -E $input_inc -P >> $config_js 
