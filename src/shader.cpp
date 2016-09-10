#include "shader.hpp"

#include <string>
#include <sstream>
#include <fstream>
#include <iostream>

using namespace std;

GLuint new_program()
{
  return glCreateProgram();
}

void program_source(GLuint program, GLenum shader_type, const string &filename)
{
  string code;

  // IO stuff
  try
  {
    stringstream sstream;
    {
      ifstream stream;
      stream.exceptions(ifstream::failbit | ifstream::badbit);
      stream.open(filename);
      sstream << stream.rdbuf();
    }
    code = sstream.str();
  }
  catch (ifstream::failure e)
  {
    cerr << "Can't open " << filename << " : " << e.what() << endl;
  }

  GLint success;
  GLchar info_log[2048];

  const char *s = code.c_str();

  // OpenGL stuff
  GLuint shad_id = glCreateShader(shader_type);
  glShaderSource(shad_id, 1, &s, NULL);
  glCompileShader(shad_id);
  glGetShaderiv(shad_id, GL_COMPILE_STATUS, &success);
  if (!success)
  {
    // error log
    glGetShaderInfoLog(shad_id, sizeof(info_log), NULL, info_log);
    cerr << "Can't compile " <<  filename << " " << info_log << endl;
    exit(-1);
  }
  glAttachShader(program, shad_id);
}

void program_link(GLuint program)
{
  GLint success;
  GLchar info_log[2048];

  glLinkProgram(program);
  glGetProgramiv(program, GL_LINK_STATUS, &success);
  if (!success)
  {
    // error log
    glGetProgramInfoLog(program, sizeof(info_log), NULL, info_log);
    cerr << "Can't link :" << info_log << endl;
  }
}