#version 450 core

layout(points) in;
layout(triangle_strip, max_vertices = 4) out;

layout (location = 8) uniform vec2 flare_size;

in vec4 pass_pos[];
in vec4 pass_col[];

out vec4 color;
out vec2 bary;

void main()
{
  vec4 pos = pass_pos[0];
  color = pass_col[0];

  vec2 f = flare_size*pass_pos[0].w;

  gl_Position = pos+vec4(-f.x,-f.y,0,0);
  bary = vec2(0,0);
  EmitVertex();

  gl_Position = pos+vec4(+f.x,-f.y,0,0);
  bary = vec2(+1,0);
  EmitVertex();

  gl_Position = pos+vec4(-f.x,+f.y,0,0);
  bary = vec2(0,+1);
  EmitVertex();

  gl_Position = pos+vec4(+f.x,+f.y,0,0);
  bary = vec2(+1,+1);
  EmitVertex();

  EndPrimitive();
}