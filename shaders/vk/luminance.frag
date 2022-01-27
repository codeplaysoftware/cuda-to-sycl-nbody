// Copyright (C) 2016 - 2018 Sarah Le Luron
#version 450 core

layout (binding = 0) uniform sampler2D tex;

out float lum;

void main(void)
{
  vec2 coords = (gl_FragCoord.xy*2+vec2(0.5));

  lum = dot(
    vec3(0.2126,0.7152, 0.0722),
    textureLod(tex, coords/textureSize(tex, 0), 0).rgb);
}