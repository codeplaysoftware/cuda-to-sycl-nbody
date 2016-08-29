#version 450 core

layout (binding = 0) uniform sampler2D tex;

layout (location = 0) uniform vec3 threshold;

out vec4 out_color;

void main()
{
  vec4 col = texelFetch(tex, ivec2(gl_FragCoord.xy), 0);
  if (any(greaterThan(col.rgb, threshold)))
    out_color = col;
  else out_color = vec4(0.0);
}