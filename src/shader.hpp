#pragma once

#include <GL/glew.h>
#include <string>

/**
 * Creates a new shader program
 * @return OpenGL handle to shader program
 */
GLuint new_program();

/**
 * Compiles a shader stage from a given source, displays errors in stderr
 * @param program shader program handle
 * @param shader_type one of GL_COMPUTE_SHADER, GL_VERTEX_SHADER, GL_TESS_CONTROL_SHADER, GL_TESS_EVALUATION_SHADER, GL_GEOMETRY_SHADER, or GL_FRAGMENT_SHADER
 * @param filename GLSL source file
 */
void program_source(GLuint program, GLenum shader_type, const std::string &filename);

/**
 * Links all shaders inside the program, displays errors in stderr
 * @param program shader program handle
 */
void program_link(GLuint program);