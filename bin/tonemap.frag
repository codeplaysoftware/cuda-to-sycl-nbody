#version 450 core

layout (binding = 0) uniform sampler2D hdr;
layout (binding = 1) uniform sampler2D bloom;

in vec2 pass_tc;

out vec4 out_color;

float exposure = 1.0;

void main()
{
  vec3 color = texelFetch(hdr, ivec2(gl_FragCoord.xy), 0).rgb;
  color += texture(bloom, pass_tc).rgb;
  vec3 tonemap = vec3(1.0)- exp(-color*exposure);

  vec3 gamma = pow(tonemap, vec3(1.0/2.2));
  out_color = vec4(gamma, 1.0);
}