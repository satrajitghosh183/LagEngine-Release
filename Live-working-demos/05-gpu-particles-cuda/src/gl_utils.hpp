#pragma once
#include <string>
#include <glad/glad.h>

GLuint compileShader(GLenum type, const std::string& src);
GLuint linkProgram(GLuint vs, GLuint fs);
GLuint loadProgramFromFiles(const char* vertPath, const char* fragPath);
std::string loadTextFile(const char* path);
void glCheckError(const char* label);
