#include "Texture2D.h"
#include <stb_image.h>
#include <iostream>
#include <utility> // std::exchange

Texture2D::Texture2D(const std::string& path)
{
    LoadFromFile(path);
}

Texture2D::~Texture2D()
{
    Destroy();
}

void Texture2D::Destroy()
{
    if (m_id != 0)
    {
        glDeleteTextures(1, &m_id);
        m_id = 0;
    }
}

Texture2D::Texture2D(Texture2D&& other) noexcept
    : m_id(std::exchange(other.m_id, 0))
{
}

Texture2D& Texture2D::operator=(Texture2D&& other) noexcept
{
    if (this == &other) return *this;
    Destroy();
    m_id = std::exchange(other.m_id, 0);
    return *this;
}

void Texture2D::Bind(GLuint slot) const
{
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(GL_TEXTURE_2D, m_id);
}

bool Texture2D::LoadFromFile(const std::string& path)
{
    // Destroy old texture if reloading
    Destroy();

    // Most images have (0,0) at top-left; OpenGL UV origin is bottom-left.
    // Flipping is usually what you want for typical PNGs.
    stbi_set_flip_vertically_on_load(1);

    int w = 0, h = 0, channels = 0;
    unsigned char* data = stbi_load(path.c_str(), &w, &h, &channels, 0);
    if (!data)
    {
        std::cerr << "Failed to load texture: " << path << "\n";
        return false;
    }

    GLenum internalFormat = GL_RGBA8;
    GLenum dataFormat = GL_RGBA;

    if (channels == 3)
    {
        internalFormat = GL_RGB8;
        dataFormat = GL_RGB;
    }
    else if (channels == 4)
    {
        internalFormat = GL_RGBA8;
        dataFormat = GL_RGBA;
    }
    else
    {
        std::cerr << "Unsupported texture channel count (" << channels
            << ") for: " << path << "\n";
        stbi_image_free(data);
        return false;
    }

    glGenTextures(1, &m_id);
    glBindTexture(GL_TEXTURE_2D, m_id);

    // Sampling & wrapping defaults (fine for now)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    // Upload
    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, w, h, 0, dataFormat, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    stbi_image_free(data);
    return true;
}
