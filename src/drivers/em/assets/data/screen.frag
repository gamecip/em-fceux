uniform highp vec3 u_lightDir;
uniform highp vec3 u_viewPos;
uniform highp vec4 u_material;
uniform highp vec3 u_fresnel;
uniform highp vec4 u_shadowPlane;
uniform sampler2D u_stretchTex;
uniform sampler2D u_noiseTex;
varying highp vec2 v_uv;
varying highp vec3 v_norm;
varying highp vec3 v_pos;
varying highp vec2 v_noiseUV;
void main ()
{
  lowp vec3 color_1;
  lowp vec4 tmpvar_2;
  tmpvar_2 = texture2D (u_stretchTex, v_uv);
  color_1 = (tmpvar_2.xyz * tmpvar_2.xyz);
  highp vec3 tmpvar_3;
  tmpvar_3 = normalize(v_norm);
  lowp vec4 tmpvar_4;
  tmpvar_4 = texture2D (u_stretchTex, (v_uv - (0.021 * tmpvar_3.xy)));
  color_1 = (color_1 + ((0.018 * tmpvar_4.xyz) * tmpvar_4.xyz));
  highp vec2 tmpvar_5;
  tmpvar_5 = max ((abs(
    (v_uv - 0.5)
  ) - vec2(0.4814453, 0.484375)), 0.0);
  highp float tmpvar_6;
  tmpvar_6 = clamp ((3.0 - (9000.0 * 
    dot (tmpvar_5, tmpvar_5)
  )), 0.0, 1.0);
  color_1 = (color_1 * tmpvar_6);
  highp float result_7;
  highp vec3 tmpvar_8;
  tmpvar_8 = normalize((u_lightDir + normalize(
    (u_viewPos - v_pos)
  )));
  highp float tmpvar_9;
  tmpvar_9 = dot (tmpvar_3, u_lightDir);
  if ((tmpvar_9 > 0.0)) {
    result_7 = (mix (u_material.x, (u_material.y * 
      pow (max (dot (tmpvar_3, tmpvar_8), 0.0), u_material.z)
    ), (u_fresnel.x + 
      (u_fresnel.y * pow ((1.0 - tmpvar_9), u_fresnel.z))
    )) * tmpvar_9);
  } else {
    result_7 = (-(tmpvar_9) * u_material.w);
  };
  result_7 = (result_7 * (0.21 + (0.79 * 
    clamp ((38.0 * max ((
      dot (u_shadowPlane.xyz, v_pos)
     - u_shadowPlane.w), (v_pos.z - 0.023))), 0.0, 1.0)
  )));
  lowp vec4 tmpvar_10;
  tmpvar_10.w = 1.0;
  tmpvar_10.xyz = (sqrt((color_1 + 
    (result_7 * ((0.638 * tmpvar_6) + 0.362))
  )) + (0.01171875 * (texture2D (u_noiseTex, v_noiseUV).x - 0.5)));
  gl_FragColor = tmpvar_10;
}

