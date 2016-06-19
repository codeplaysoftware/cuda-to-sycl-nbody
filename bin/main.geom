#version 450 core

layout(points) in;
layout(triangle_strip, max_vertices = 4) out;

layout (location = 4) uniform mat4 p;

#define M_PI 3.1415926535897932384626433832795

void main()
{
  vec4 pos = vec4(gl_in[0].gl_Position.xyz, 1.0);
  float mass = gl_in[0].gl_Position.w;
  float radius = exp(log(mass*3/(4*M_PI))/3)*0.01;

  gl_Position = p*(pos+vec4(-radius, -radius,0.0,0.0));
  EmitVertex();

  gl_Position = p*(pos+vec4(+radius, -radius,0.0,0.0));
  EmitVertex();

  gl_Position = p*(pos+vec4(-radius, +radius,0.0,0.0));
  EmitVertex();

  gl_Position = p*(pos+vec4(+radius, +radius,0.0,0.0));
  EmitVertex();

  EndPrimitive();
}