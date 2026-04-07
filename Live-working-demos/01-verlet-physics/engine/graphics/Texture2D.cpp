#include "Texture2D.hpp"
#include <glad/glad.h>
#include <stb_image.h>
#include "engine/core/Logger.hpp"

namespace engine::graphics {

Texture2D::Texture2D(const std::string& path)
    : textureID(0), width(0), height(0), channels(0)
{
    using namespace engine::core::log;

    stbi_set_flip_vertically_on_load(true); // Flip because OpenGL expects (0,0) bottom left
    unsigned char* data = stbi_load(path.c_str(), &width, &height, &channels, 0);

    if (!data) {
        Logger::log("Failed to load texture: " + path, LogLevel::Error);
        return;
    }

    GLenum format = GL_RGB;
    if (channels == 1)
        format = GL_RED;
    else if (channels == 3)
        format = GL_RGB;
    else if (channels == 4)
        format = GL_RGBA;

    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    // Texture settings (feel free to tweak for better cloth look)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR); 
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glBindTexture(GL_TEXTURE_2D, 0);
    stbi_image_free(data);

    Logger::log("Loaded texture: " + path + " (" + std::to_string(width) + "x" + std::to_string(height) + ")", LogLevel::Info);
}

Texture2D::~Texture2D() {
    if (textureID != 0) {
        glDeleteTextures(1, &textureID);
    }
}

void Texture2D::bind(unsigned int slot) const {
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(GL_TEXTURE_2D, textureID);
}

void Texture2D::unbind() const {
    glBindTexture(GL_TEXTURE_2D, 0);
}

} // namespace engine::graphics
