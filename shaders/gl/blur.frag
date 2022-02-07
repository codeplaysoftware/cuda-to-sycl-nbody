// Copyright (C) 2016 - 2018 Sarah Le Luron
#version 450 core

layout (binding = 0) uniform sampler2D tex;

layout (location = 0) uniform vec2 size;
layout (location = 1) uniform vec2 mult;
layout (location = 2) uniform int kHalfWidth;
// Maximum length of gauss kernel sample = 100
layout (location = 3) uniform float[100] offset;
layout (location = 103) uniform float[100] weight;

in vec2 pass_tc;

out vec4 out_color;

vec4 contribute(float offset, float weight)
{
  return (texture(tex, pass_tc+offset*mult*size)+
          texture(tex, pass_tc-offset*mult*size))
           *weight;
}

void main()
{
  out_color = texture(tex, pass_tc) * weight[0];
  for(int i = 1; i < kHalfWidth; i++){
    out_color += contribute(offset[i], weight[i]);
  }
}