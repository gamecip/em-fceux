#!/bin/bash
# TODO: tsone: following should be added to Scons scripts?
config_js=src/drivers/em/site/config.js
input_inc=src/drivers/em/input.inc.hpp
config_inc=src/drivers/em/config.inc.hpp
echo "// WARNING! AUTOMATICALLY GENERATED FILE. DO NOT EDIT." > $config_js
emcc -D 'CONTROLLER_PRE=' -D 'CONTROLLER_POST=' -D 'CONTROLLER(i_,d_,e_,id_)=var e_ = i_;' -E $config_inc -P >> $config_js 
emcc -D 'CONTROLLER_PRE=var FCEC = { controllers : {' -D 'CONTROLLER_POST=},' -D 'CONTROLLER(i_,d_,e_,id_)=i_ : [ d_, id_ ],' -E $config_inc -P >> $config_js 
emcc -D 'INPUT_PRE=inputs : {' -D 'INPUT_POST=},' -D 'INPUT(i_,dk_,dg_,e_,t_)=i_ : [ dk_, dg_, t_ ],' -E $input_inc -P >> $config_js 
echo -e '\n};' >> $config_js
echo "Generated $config_js"
#hack: re-build to incorporate config.js properly
emscons scons -j 4 $@
