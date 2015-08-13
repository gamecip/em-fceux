uniform sampler2D u_idxTex;
uniform sampler2D u_deempTex;
uniform sampler2D u_lookupTex;
uniform sampler2D u_noiseTex;
uniform highp vec3 u_mins;
uniform highp vec3 u_maxs;
uniform highp float u_brightness;
uniform highp float u_contrast;
uniform highp float u_color;
uniform highp float u_gamma;
uniform highp float u_noiseAmp;
varying highp vec2 v_uv;
varying highp vec2 v_deemp_uv;
varying highp vec2 v_noiseUV;
void main ()
{
  lowp vec3 yiq_1;
  lowp vec2 tmpvar_2;
  tmpvar_2.x = 1.0;
  tmpvar_2.y = ((0.4990215 * texture2D (u_idxTex, v_uv).x) + (0.9980431 * texture2D (u_deempTex, v_deemp_uv).x));
  lowp vec3 tmpvar_3;
  tmpvar_3 = ((texture2D (u_lookupTex, tmpvar_2).xyz * (u_maxs - u_mins)) + u_mins);
  yiq_1.x = (tmpvar_3.x + (u_noiseAmp * (texture2D (u_noiseTex, v_noiseUV).x - 0.5)));
  yiq_1.yz = (tmpvar_3.yz * u_color);
  lowp vec4 tmpvar_4;
  tmpvar_4.w = 1.0;
  tmpvar_4.xyz = clamp (((u_contrast * 
    pow (clamp ((mat3(1.0, 1.0, 1.0, 0.946882, -0.274788, -1.108545, 0.623557, -0.635691, 1.709007) * yiq_1), 0.0, 1.0), vec3(u_gamma))
  ) + u_brightness), 0.0, 1.0);
  gl_FragColor = tmpvar_4;
}

