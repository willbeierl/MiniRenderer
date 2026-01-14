#pragma once
#include <cstddef>
#include <glad/glad.h>
#include "VertexArray.h"
#include "Buffer.h"

class Mesh
{
public:
    Mesh(const float* vertices, std::size_t vBytes,
        const unsigned int* indices, std::size_t iBytes,
        int indexCount);

    void Draw() const;
    int IndexCount() const { return m_indexCount; }

private:
    VertexArray m_vao;
    Buffer m_vbo;
    Buffer m_ebo;
    int m_indexCount = 0;
};
