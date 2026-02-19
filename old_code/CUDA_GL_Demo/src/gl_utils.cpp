#include "gl_utils.hpp"
#include <fstream>
#include <sstream>
#include <iostream>

std::string loadTextFile(const char* path) {
    std::ifstream file(path);
    if(!file) throw std::runtime_error(std::string("Failed to open: ")+path);
    std::stringstream ss; ss << file.rdbuf();
    return ss.str();
}

GLuint compileShader(GLenum type, const std::string& src) {
    GLuint sh = glCreateShader(type);
    const char* csrc = src.c_str();
    glShaderSource(sh, 1, &csrc, nullptr);
    glCompileShader(sh);
    GLint ok = 0; glGetShaderiv(sh, GL_COMPILE_STATUS, &ok);
    if(!ok) {
        GLint len=0; glGetShaderiv(sh, GL_INFO_LOG_LENGTH, &len);
        std::string log(len, '\0');
        glGetShaderInfoLog(sh, len, nullptr, log.data());
        std::cerr << "Shader compile error:\n" << log << std::endl;
        throw std::runtime_error("Shader compile failed");
    }
    return sh;
}

GLuint linkProgram(GLuint vs, GLuint fs) {
    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);
    GLint ok = 0; glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if(!ok) {
        GLint len=0; glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &len);
        std::string log(len, '\0');
        glGetProgramInfoLog(prog, len, nullptr, log.data());
        std::cerr << "Program link error:\n" << log << std::endl;
        throw std::runtime_error("Program link failed");
    }
    glDetachShader(prog, vs);
    glDetachShader(prog, fs);
    glDeleteShader(vs);
    glDeleteShader(fs);
    return prog;
}

GLuint loadProgramFromFiles(const char* vertPath, const char* fragPath) {
    auto vsrc = loadTextFile(vertPath);
    auto fsrc = loadTextFile(fragPath);
    GLuint vs = compileShader(GL_VERTEX_SHADER, vsrc);
    GLuint fs = compileShader(GL_FRAGMENT_SHADER, fsrc);
    return linkProgram(vs, fs);
}

void glCheckError(const char* label) {
#ifdef NDEBUG
    (void)label;
#else
    for(GLenum err; (err = glGetError()) != GL_NO_ERROR;) {
        std::cerr << "[GL ERROR] (" << label << ") code=" << std::hex << err << std::dec << "\n";
    }
#endif
}
