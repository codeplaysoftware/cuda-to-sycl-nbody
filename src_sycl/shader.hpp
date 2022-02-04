// Copyright (C) 2016 - 2018 Sarah Le Luron

#pragma once

#include <GL/glew.h>

#include <string>

class ShaderProgram {
  public:
   ShaderProgram();

   /**
    * Compiles a shader stage from a given source, displays errors in stderr
    * @param program shader program handle
    * @param shader_type one of GL_COMPUTE_SHADER, GL_VERTEX_SHADER,
    * GL_TESS_CONTROL_SHADER, GL_TESS_EVALUATION_SHADER, GL_GEOMETRY_SHADER, or
    * GL_FRAGMENT_SHADER
    * @param filename GLSL source file
    */
   void source(GLenum shaderType, const std::string &filename);

   /**
    * Links all shaders inside the program, displays errors in stderr
    */
   void link();

   GLuint getId();

  private:
   GLuint id;
};