#pragma once
#include <glad/glad.h>
#include <cstddef>

class Buffer
{
public:
    explicit Buffer(GLenum target);
    ~Buffer();

    Buffer(const Buffer&) = delete;
    Buffer& operator=(const Buffer&) = delete;

    Buffer(Buffer&& other) noexcept;
    Buffer& operator=(Buffer&& other) noexcept;

    void Bind() const;
    static void Unbind(GLenum target);

    void SetData(const void* data, std::size_t sizeBytes, GLenum usage) const;

    GLuint Id() const { return m_id; }
    GLenum Target() const { return m_target; }

private:
    void Destroy();

    GLuint m_id = 0;
    GLenum m_target = 0;
};
