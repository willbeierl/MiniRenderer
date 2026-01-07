#pragma once

#include <string>
#include <glad/glad.h>

class ShaderProgram
{
public:
    ShaderProgram() = default;

    // Construct from shader paths
    ShaderProgram(std::string vertexPath, std::string fragmentPath);

    // RAII: destructor releases GPU program
    ~ShaderProgram();

    // Non-copyable (important)
    ShaderProgram(const ShaderProgram&) = delete;
    ShaderProgram& operator=(const ShaderProgram&) = delete;

    // Moveable (important)
    ShaderProgram(ShaderProgram&& other) noexcept;
    ShaderProgram& operator=(ShaderProgram&& other) noexcept;

    void Use() const;

    // Recompile/relink from the stored file paths
    // Returns true if new program successfully replaced the old one.
    bool Reload();

    GLuint Id() const { return m_id; }

private:
    void Destroy(); // helper: deletes m_id if valid and sets to 0

    GLuint m_id = 0;
    std::string m_vertexPath;
    std::string m_fragmentPath;
};
