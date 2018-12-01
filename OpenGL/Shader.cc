#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "Shader.h"

#include <iostream>
#include <fstream>
using namespace std;

unsigned int CShader::create_shader_by_string(const char *shader_source, GLenum type)
{
	// 创建Shader
	unsigned int shader = glCreateShader(type);
	// 设置源码
	glShaderSource(shader, 1, &shader_source, NULL);
	// 编译Shader
	glCompileShader(shader);

	// 检查错误
	int success; char log_buffer[512];
	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(shader, 512, NULL, log_buffer);

		const char *type_info = NULL;
		if (type == GL_VERTEX_SHADER) {
			type_info = "VERTEX";
		}
		else if (type == GL_VERTEX_SHADER) {
			type_info = "FRAGMENT";
		}
		else {
			type_info = "UNKNOWN";
		}
		cout << "ERROR::SHADER::" << type_info << "::COMPILATION_FAILED\n" << log_buffer << endl;
		return 0;
	}

	return shader;
}

unsigned int CShader::create_shader_by_file(const char *fname, GLenum type)
{
	auto shader_file = ifstream(fname, ios::in);

	istreambuf_iterator<char> beg(shader_file), end;

	string content(beg, end);

	return create_shader_by_string(content.c_str(), type);
}

CShader::CShader(const char *vertex_shader_path, const char *fragment_shader_path)
{
	auto vshader = create_shader_by_file(vertex_shader_path, GL_VERTEX_SHADER);
	auto fshader = create_shader_by_file(fragment_shader_path, GL_FRAGMENT_SHADER);

	_shader_id = glCreateProgram();
	glAttachShader(_shader_id, vshader);
	glAttachShader(_shader_id, fshader);
	glLinkProgram(_shader_id);

	int success; char info_log[512];
	glGetProgramiv(_shader_id, GL_LINK_STATUS, &success);
	if (!success)
	{
		glGetProgramInfoLog(_shader_id, 512, NULL, info_log);
		cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << info_log << endl;
		return;
	}

	glDeleteShader(vshader);
	glDeleteShader(fshader);
}