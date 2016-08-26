#version 450 core

layout(points) in;
layout(triangle_strip, max_vertices = 4) out;

layout (location = 4) uniform mat4 p;

in vec4 pass_pos[];
in vec4 pass_col[];

out vec4 color;
out vec2 bary;

void main()
{
  vec4 pos = vec4(pass_pos[0].xyz, 1.0);
  float r = 0.2;

  color = pass_col[0];

  gl_Position = p*(pos+vec4(-r,-r,0,0));
  bary = vec2(0,0);
  EmitVertex();

  gl_Position = p*(pos+vec4(+r,-r,0,0));
  bary = vec2(+1,0);
  EmitVertex();

  gl_Position = p*(pos+vec4(-r,+r,0,0));
  bary = vec2(0,+1);
  EmitVertex();

  gl_Position = p*(pos+vec4(+r,+r,0,0));
  bary = vec2(+1,+1);
  EmitVertex();

  EndPrimitive();
}