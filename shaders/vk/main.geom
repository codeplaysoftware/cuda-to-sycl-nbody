// Copyright (C) 2016 - 2018 Sarah Le Luron
#version 450 core
#extension GL_ARB_separate_shader_objects : enable

layout(points) in;
layout(triangle_strip, max_vertices = 4) out;

layout (binding=0) uniform staticParam
{
  vec2 flareSize;
};

out gl_PerVertex
{
  vec4 gl_Position;
};

in vec4 pass_pos[];
in vec4 pass_col[];

layout (location = 0) out vec4 color;
layout (location = 1) out vec2 bary;

void main()
{
  vec4 pos = pass_pos[0];
  color = pass_col[0];

  vec2 f = flareSize*pass_pos[0].w;

  gl_Position = pos+vec4(-f.x,-f.y,0,0);
  bary = vec2(0,0);
  EmitVertex();

  gl_Position = pos+vec4(+f.x,-f.y,0,0);
  bary = vec2(1,0);
  EmitVertex();

  gl_Position = pos+vec4(-f.x,+f.y,0,0);
  bary = vec2(0,1);
  EmitVertex();

  gl_Position = pos+vec4(+f.x,+f.y,0,0);
  bary = vec2(1,1);
  EmitVertex();

  EndPrimitive();
}