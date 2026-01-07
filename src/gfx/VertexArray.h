#pragma once
#include <glad/glad.h>
#include <cstddef>

class VertexArray
{
public:
    VertexArray();
    ~VertexArray();

    VertexArray(const VertexArray&) = delete;
    VertexArray& operator=(const VertexArray&) = delete;

    VertexArray(VertexArray&& other) noexcept;
    VertexArray& operator=(VertexArray&& other) noexcept;

    void Bind() const;
    static void Unbind();

    void SetAttribute(
        GLuint index,
        GLint size,
        GLenum type,
        GLboolean normalized,
        GLsizei strideBytes,
        std::size_t offsetBytes
    ) const;

    GLuint Id() const { return m_id; }

private:
    void Destroy();
    GLuint m_id = 0;
};
