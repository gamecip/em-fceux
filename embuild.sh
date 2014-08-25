#!/bin/bash
pushd ~/emsdk && source emsdk_env.sh && popd
rm -f src/fceux.data src/fceux.js* src/fceux.html*
emscons scons $@
