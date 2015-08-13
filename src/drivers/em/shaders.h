static const char* common_src =
"precision highp float;\n"
#if DBG_MODE
"uniform vec3 u_mouse;\n"
#else
""
#endif
;
