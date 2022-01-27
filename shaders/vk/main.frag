// Copyright (C) 2016 - 2018 Sarah Le Luron
#version 450 core
#extension GL_ARB_separate_shader_objects : enable

layout (location = 0) in vec4 color;
layout (location = 1) in vec2 bary;

layout (binding=1) uniform sampler2D tex;

layout (location=0) out vec4 out_color;

void main()
{
  float alpha = texture(tex, bary).r;
  out_color = vec4(color.rgb*alpha, 1.0);
}