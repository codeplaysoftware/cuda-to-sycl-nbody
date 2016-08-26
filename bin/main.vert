#version 450 core

layout (location = 0) uniform mat4 mv;
layout (location = 4) uniform mat4 p;

layout (location = 0) in vec4 pos;
layout (location = 1) in vec4 col;

out vec4 pass_pos;
out vec4 pass_col;

void main()
{
  pass_pos = p*mv*pos;
  pass_col = col;
}