#include "renderer.hpp"
#include "gl_utils.hpp"
#include <iostream>

static GLuint createVBO(GLsizeiptr bytes, GLenum usage) {
    GLuint id=0; glGenBuffers(1, &id);
    glBindBuffer(GL_ARRAY_BUFFER, id);
    glBufferData(GL_ARRAY_BUFFER, bytes, nullptr, usage);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    return id;
}

bool Renderer::init(size_t particleCount) {
    program = loadProgramFromFiles("particles.vert", "particles.frag");

    const GLsizeiptr posBytes = static_cast<GLsizeiptr>(particleCount * sizeof(float) * 4);
    const GLsizeiptr colBytes = static_cast<GLsizeiptr>(particleCount * sizeof(float) * 4);
    vboPositions = createVBO(posBytes, GL_DYNAMIC_DRAW);
    vboColors    = createVBO(colBytes, GL_DYNAMIC_DRAW);

    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, vboPositions);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(float)*4, (void*)0);

    glBindBuffer(GL_ARRAY_BUFFER, vboColors);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(float)*4, (void*)0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glCheckError("Renderer::init");
    
    std::cout << "OpenGL Renderer initialized\n";
    std::cout << "  Vendor: " << glGetString(GL_VENDOR) << "\n";
    std::cout << "  Renderer: " << glGetString(GL_RENDERER) << "\n";
    std::cout << "  Version: " << glGetString(GL_VERSION) << "\n\n";
    
    return true;
}

void Renderer::draw(size_t particleCount) {
    glUseProgram(program);
    glBindVertexArray(vao);

    glEnable(GL_PROGRAM_POINT_SIZE);
    glDrawArrays(GL_POINTS, 0, static_cast<GLsizei>(particleCount));

    glBindVertexArray(0);
    glUseProgram(0);
}

void Renderer::shutdown() {
    if (vboPositions) glDeleteBuffers(1, &vboPositions);
    if (vboColors)    glDeleteBuffers(1, &vboColors);
    if (vao)          glDeleteVertexArrays(1, &vao);
    if (program)      glDeleteProgram(program);
    vao=vboPositions=vboColors=program=0;
}
