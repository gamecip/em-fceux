attribute vec4 a_0;
attribute vec2 a_2;
uniform vec2 u_noiseRnd;
varying vec2 v_uv;
varying vec2 v_deemp_uv;
varying vec2 v_noiseUV;
void main ()
{
  v_uv = a_2;
  vec2 tmpvar_1;
  tmpvar_1.y = 0.0;
  tmpvar_1.x = a_2.y;
  v_deemp_uv = tmpvar_1;
  v_noiseUV = ((vec2(1.09375, 0.875) * a_2) + u_noiseRnd);
  gl_Position = a_0;
}

