#!/bin/bash
# Huh?? Why we suddenly start needing pwd here?
emscons scons -j 4 $@
# TODO: tsone: following should be added to Scons scripts?
config_js=src/drivers/em/site/config.js
input_inc=src/drivers/em/input.inc.hpp
config_inc=src/drivers/em/config.inc.hpp
echo "// WARNING! AUTOMATICALLY GENERATED FILE. DO NOT EDIT." > $config_js
emcc -D 'CONTROLLER_PRE=' -D 'CONTROLLER_POST=' -D 'CONTROLLER(i_,d_,e_,id_)=var e_ = i_;' -E $config_inc -P >> $config_js 
emcc -D 'CONTROLLER_PRE=var FCEC = { controllers : {' -D 'CONTROLLER_POST=},' -D 'CONTROLLER(i_,d_,e_,id_)=i_ : [ d_, id_ ],' -E $config_inc -P >> $config_js 
emcc -D 'INPUT_PRE=inputs : {' -D 'INPUT_POST=},' -D 'INPUT(i_,dk_,dg_,e_,t_)=i_ : [ dk_, dg_, t_ ],' -E $input_inc -P >> $config_js 
# Save fceux.js file size to config.js for progress bar size when gzip transfer encoding is used.
fceux_js=src/drivers/em/site/fceux.js
python -c "import os; print 'FCEUX_JS_SIZE : %s,\n};' % (os.stat('$fceux_js').st_size)" >> $config_js
echo "Generated $config_js"
