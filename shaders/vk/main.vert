#version 450 core
#extension GL_ARB_separate_shader_objects : enable

layout (push_constant) uniform vertpushConstants
{
  mat4 mv;
  mat4 p;
} pushConstants;

layout (location = 0) in vec4 pos;
layout (location = 1) in vec4 vel;

out vec4 pass_pos;
out vec4 pass_col;

void main()
{
  pass_pos = pushConstants.p*pushConstants.mv*pos;

  // slow->blue, fast->purple
  vec3 color = mix(vec3(0,0.4,1),vec3(1,0.2,1),clamp(dot(vel,vel)*0.0006,0,1));

  pass_col = vec4(color,1.0);
}