#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "Shader.h"

#include <iostream>
#include <fstream>
using namespace std;

unsigned int CShader::glCreateShaderByString(const char *source, GLenum type)
{
	// 创建Shader
	unsigned int shader = glCreateShader(type);
	// 设置源码
	glShaderSource(shader, 1, &source, NULL);
	// 编译Shader
	glCompileShader(shader);

	// 检查错误
	int success; char logBuffer[512];
	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(shader, 512, NULL, logBuffer);

		const char *typeInfo = NULL;
		if (type == GL_VERTEX_SHADER) {
			typeInfo = "VERTEX";
		}
		else if (type == GL_VERTEX_SHADER) {
			typeInfo = "FRAGMENT";
		}
		else {
			typeInfo = "UNKNOWN";
		}
		cout << "ERROR::SHADER::" << typeInfo << "::COMPILATION_FAILED\n" << logBuffer << endl;
		return 0;
	}

	return shader;
}

unsigned int CShader::glCreateShaderByFile(const char *fname, GLenum type)
{
	auto shaderFile = ifstream(fname, ios::in);

	istreambuf_iterator<char> beg(shaderFile), end;

	string content(beg, end);

	return glCreateShaderByString(content.c_str(), type);
}

CShader::CShader(const char *vertexShaderPath, const char *fragmentShaderPath)
{
	auto vshader = glCreateShaderByFile(vertexShaderPath, GL_VERTEX_SHADER);
	auto fshader = glCreateShaderByFile(fragmentShaderPath, GL_FRAGMENT_SHADER);

	_shaderId = glCreateProgram();
	glAttachShader(_shaderId, vshader);
	glAttachShader(_shaderId, fshader);
	glLinkProgram(_shaderId);

	int success; char infoLog[512];
	glGetProgramiv(_shaderId, GL_LINK_STATUS, &success);
	if (!success)
	{
		glGetProgramInfoLog(_shaderId, 512, NULL, infoLog);
		cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << endl;
		return;
	}

	glDeleteShader(vshader);
	glDeleteShader(fshader);
}