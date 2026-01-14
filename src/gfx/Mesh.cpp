#include "Mesh.h"

Mesh::Mesh(const float* vertices, std::size_t vBytes,
    const unsigned int* indices, std::size_t iBytes,
    int indexCount)
    : m_vbo(GL_ARRAY_BUFFER),
    m_ebo(GL_ELEMENT_ARRAY_BUFFER),
    m_indexCount(indexCount)
{
    m_vao.Bind();

    m_vbo.Bind();
    m_vbo.SetData(vertices, vBytes, GL_STATIC_DRAW);

    m_ebo.Bind();
    m_ebo.SetData(indices, iBytes, GL_STATIC_DRAW);

    // Layout: pos(3), normal(3), uv(2) = 8 floats
    m_vao.SetAttribute(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), 0);
    m_vao.SetAttribute(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), 3 * sizeof(float));
    m_vao.SetAttribute(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), 6 * sizeof(float));

    VertexArray::Unbind();
    Buffer::Unbind(GL_ARRAY_BUFFER);
}

void Mesh::Draw() const
{
    m_vao.Bind();
    glDrawElements(GL_TRIANGLES, m_indexCount, GL_UNSIGNED_INT, (void*)0);
}
