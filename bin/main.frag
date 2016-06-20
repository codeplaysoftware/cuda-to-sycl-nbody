#version 450 core

in vec2 bary;

out vec4 color;

void main()
{
  if (dot(bary,bary)>1)
    discard;
  color = vec4(1.0);
}