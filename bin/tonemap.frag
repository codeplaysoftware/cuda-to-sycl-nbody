#version 450 core

const int FBO_MARGIN = 50;

layout (binding = 0) uniform sampler2D hdr;
layout (binding = 1) uniform sampler2D bloom;
layout (binding = 2) uniform sampler2D lum;

layout (location = 0) uniform int lum_lod;

in vec2 pass_tc;

out vec4 out_color;

void main()
{
  ivec2 coord = ivec2(gl_FragCoord.xy)+ivec2(FBO_MARGIN);

  vec3 color = texelFetch(hdr,coord,0).rgb;

  float luminance = textureLod(lum, vec2(0.5), lum_lod).r;
  float exposure = 1.0/luminance;

  color += texture(bloom, vec2(coord)/textureSize(hdr, 0)).rgb;
  vec3 tonemap = vec3(1.0)- exp(-color*exposure);

  vec3 gamma = pow(tonemap, vec3(1.0/2.2));
  out_color = vec4(gamma, 1.0);
}