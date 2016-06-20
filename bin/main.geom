#version 450 core

layout(points) in;
layout(triangle_strip, max_vertices = 4) out;

layout (location = 4) uniform mat4 p;

in vec4 posrad[];
out vec2 bary;

void main()
{
  vec4 pos = vec4(posrad[0].xyz, 1.0);
  float radius = posrad[0].w;

  gl_Position = p*(pos+vec4(-radius, -radius,0.0,0.0));
  bary = vec2(-1,-1);
  EmitVertex();

  gl_Position = p*(pos+vec4(+radius, -radius,0.0,0.0));
  bary = vec2(+1,-1);
  EmitVertex();

  gl_Position = p*(pos+vec4(-radius, +radius,0.0,0.0));
  bary = vec2(-1,+1);
  EmitVertex();

  gl_Position = p*(pos+vec4(+radius, +radius,0.0,0.0));
  bary = vec2(+1,+1);
  EmitVertex();

  EndPrimitive();
}