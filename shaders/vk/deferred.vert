# Copyright (C) 2016 - 2018 Sarah Le Luron
#version 450 core

layout (location = 0) in vec2 in_pos;

out vec2 pass_tc;

void main()
{
  gl_Position = vec4(in_pos,0.0,1.0);
  pass_tc = in_pos*0.5+vec2(0.5);
}