#include "VertexArray.h"
#include <utility> // std::exchange

VertexArray::VertexArray()
{
    glGenVertexArrays(1, &m_id);
}

VertexArray::~VertexArray()
{
    Destroy();
}

void VertexArray::Destroy()
{
    if (m_id != 0)
    {
        glDeleteVertexArrays(1, &m_id);
        m_id = 0;
    }
}

void VertexArray::Bind() const
{
    glBindVertexArray(m_id);
}

void VertexArray::Unbind()
{
    glBindVertexArray(0);
}

void VertexArray::SetAttribute(
    GLuint index,
    GLint size,
    GLenum type,
    GLboolean normalized,
    GLsizei strideBytes,
    std::size_t offsetBytes
) const
{
    glVertexAttribPointer(
        index,
        size,
        type,
        normalized,
        strideBytes,
        reinterpret_cast<const void*>(offsetBytes)
    );
    glEnableVertexAttribArray(index);
}

VertexArray::VertexArray(VertexArray&& other) noexcept
    : m_id(std::exchange(other.m_id, 0))
{
}

VertexArray& VertexArray::operator=(VertexArray&& other) noexcept
{
    if (this == &other) return *this;
    Destroy();
    m_id = std::exchange(other.m_id, 0);
    return *this;
}
