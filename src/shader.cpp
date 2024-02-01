// Copyright (C) 2016 - 2018 Sarah Le Luron

#include "shader.hpp"

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

using namespace std;

ShaderProgram::ShaderProgram() : id(0) {}

void ShaderProgram::source(GLenum shader_type, const string &filename) {
  if (!id) id = glCreateProgram();

  string code;

  // IO stuff
  try {
    stringstream sstream;
    {
      ifstream stream;
      stream.exceptions(ifstream::failbit | ifstream::badbit);
      stream.open(filename);
      sstream << stream.rdbuf();
    }
    code = sstream.str();
  } catch (ifstream::failure e) {
    throw std::runtime_error(std::string("Can't open ") + filename +
        std::string(e.what()));
  }

  GLint success;
  GLchar info_log[2048];

  const char *s = code.c_str();

  // OpenGL stuff
  GLuint shad_id = glCreateShader(shader_type);
  glShaderSource(shad_id, 1, &s, NULL);
  glCompileShader(shad_id);
  glGetShaderiv(shad_id, GL_COMPILE_STATUS, &success);
  if (!success) {
    // error log
    glGetShaderInfoLog(shad_id, sizeof(info_log), NULL, info_log);
    throw std::runtime_error(std::string("Can't compile ") + filename + " " +
        info_log);
    exit(-1);
  }
  glAttachShader(id, shad_id);
}

void ShaderProgram::link() {
  GLint success;
  GLchar info_log[2048];

  glLinkProgram(id);
  glGetProgramiv(id, GL_LINK_STATUS, &success);
  if (!success) {
    // error log
    glGetProgramInfoLog(id, sizeof(info_log), NULL, info_log);
    throw std::runtime_error(std::string("Can't link ") +
        std::string(info_log));
  }
}

GLuint ShaderProgram::getId() { return id; }
