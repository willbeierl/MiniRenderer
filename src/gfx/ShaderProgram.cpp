#include <string>
#include "ShaderProgram.h"
#include <iostream>
#include <utility>

std::string LoadTextFile(const std::string& path);
GLuint CreateProgram(const char* vsSource, const char* fsSource);

ShaderProgram::ShaderProgram(std::string vertexPath, std::string fragmentPath)
	:	m_vertexPath(std::move(vertexPath)),
		m_fragmentPath(std::move(fragmentPath))
{
	Reload(); // if it fails, m_id will remain 0
}

bool ShaderProgram::Reload()
{
	std::string vs = LoadTextFile(m_vertexPath);
	std::string fs = LoadTextFile(m_fragmentPath);

	if (vs.empty() || fs.empty())
	{
		std::cerr << "[Reload] Shader file was empty or missing.\n";
		return false;
	}

	GLuint newProgram = CreateProgram(vs.c_str(), fs.c_str());

	if (newProgram == 0)
	{
		std::cerr << "[Reload] Compile/link failed. Keeping previous shader.\n";
		return false;
	}

	// success: replace the program
	Destroy();
	m_id = newProgram;
	
	std::cout << "[Reload] Shaders reloaded successfully.\n";
	return true;
}

void ShaderProgram::Use() const
{
	if (m_id != 0)
		glUseProgram(m_id);
}

void ShaderProgram::Destroy()
{
	if (m_id != 0)
	{
		glDeleteProgram(m_id);
		m_id = 0;
	}
}

ShaderProgram::~ShaderProgram()
{
	Destroy();
}