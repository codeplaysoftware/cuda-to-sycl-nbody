#version 450 core

layout (binding = 0) uniform sampler2D tex;

layout (location = 0) uniform vec3 threshold;

out vec4 out_color;

void main()
{
  const vec4 MAX = vec4(100,100,100,1.0);
  vec4 col = texelFetch(tex, ivec2(gl_FragCoord.xy), 0);
  if (any(greaterThan(col.rgb, threshold)))
    out_color = min(col, MAX);
  else out_color = vec4(0.0);
}