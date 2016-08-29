#version 450 core

layout (location = 0) uniform mat4 mv;
layout (location = 4) uniform mat4 p;

layout (location = 0) in vec4 pos;
layout (location = 1) in vec4 vel;

out vec4 pass_pos;
out vec4 pass_col;

void main()
{
  pass_pos = p*mv*pos;

  // slow->blue, fast->purple
  vec3 color = mix(vec3(0,2.0,5.0),vec3(10,2,10),clamp(dot(vel,vel)*0.001,0,1));
  float towards = -(mv*vel).z*0.0005;
  // subtle doppler
  color *= mix(vec3(0.0,5.0,5.0),vec3(5.0,0.0,0.0),towards+0.5);
  pass_col = vec4(color,1.0);
}