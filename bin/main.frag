#version 450 core

in vec2 bary;
in vec4 color;

layout (binding = 0) uniform sampler2D tex;

out vec4 out_color;

void main()
{
  float alpha = texture(tex, bary).r;
  out_color = vec4(color.rgb*alpha, 1.0);
}