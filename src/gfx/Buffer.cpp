#include "Buffer.h"
#include <utility> // std::exchange

Buffer::Buffer(GLenum target)
	:	m_target(target)
{
	glGenBuffers(1, &m_id);
}

Buffer::~Buffer()
{
	Destroy();
}

void Buffer::Destroy()
{
	if (m_id != 0)
	{
		glDeleteBuffers(1, &m_id);
		m_id = 0;
	}
}

void Buffer::Bind() const
{
	glBindBuffer(m_target, m_id);
}

void Buffer::Unbind(GLenum target)
{
	glBindBuffer(target, 0);
}

void Buffer::SetData(const void* data, std::size_t sizeBytes, GLenum usage) const
{
	// Make sure this buffer is bound before uploading
	Bind();
	glBufferData(m_target, static_cast<GLsizeiptr>(sizeBytes), data, usage);
}

