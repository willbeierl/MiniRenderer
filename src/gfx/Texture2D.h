#pragma once
#include <glad/glad.h>
#include <string>

class Texture2D
{
public:
    Texture2D() = default;
    explicit Texture2D(const std::string& path);
    ~Texture2D();

    Texture2D(const Texture2D&) = delete;
    Texture2D& operator=(const Texture2D&) = delete;

    Texture2D(Texture2D&& other) noexcept;
    Texture2D& operator=(Texture2D&& other) noexcept;

    bool LoadFromFile(const std::string& path);

    void Bind(GLuint slot = 0) const;
    GLuint Id() const { return m_id; }

private:
    void Destroy();
    GLuint m_id = 0;
};
