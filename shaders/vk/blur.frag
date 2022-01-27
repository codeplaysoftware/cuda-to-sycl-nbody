// Copyright (C) 2016 - 2018 Sarah Le Luron
#version 450 core

layout (binding = 0) uniform sampler2D tex;

layout (location = 0) uniform vec2 size;
layout (location = 1) uniform vec2 mult;

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
  out_color = texture(tex, pass_tc) * 0.22702703;
  out_color += contribute(1.38461538, 0.31621622);
  out_color += contribute(3.23076923, 0.07027027);
}