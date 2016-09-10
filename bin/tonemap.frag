#version 450 core

const int FBO_MARGIN = 50;

layout (binding = 0) uniform sampler2D hdr;
layout (binding = 1) uniform sampler2D bloom;

in vec2 pass_tc;

out vec4 out_color;

float exposure = 4.0;

void main()
{
  ivec2 coord = ivec2(gl_FragCoord.xy)+ivec2(FBO_MARGIN);

  vec3 color = texelFetch(hdr,coord,0).rgb;

  color += texture(bloom, vec2(coord)/textureSize(hdr, 0)).rgb;
  vec3 tonemap = vec3(1.0)- exp(-color*exposure);

  vec3 gamma = pow(tonemap, vec3(1.0/2.2));
  out_color = vec4(gamma, 1.0);
}