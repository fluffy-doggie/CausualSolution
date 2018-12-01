#ifndef __C_SHADER_H__
#define __C_SHADER_H__

class CShader
{
public:
	static unsigned int create_shader_by_file(const char *, GLenum);
	static unsigned int create_shader_by_string(const char *, GLenum);

	CShader(const char *vertex_shader_path, const char *fragment_shader_path);
	~CShader();

	void use() {
		glUseProgram(_shader_id);
	}
	
	// 工具函数：设置uniform参数值
	void uniform(const char *name, bool value) {
		glUniform1i(glGetUniformLocation(_shader_id, name), int(value));
	}

	void uniform(const char *name, int value) {
		glUniform1i(glGetUniformLocation(_shader_id, name), value);
	}

	void uniform(const char *name, float a) {
		glUniform1f(glGetUniformLocation(_shader_id, name), a);
	}
	void uniform(const char *name, float a, float b) {
		glUniform2f(glGetUniformLocation(_shader_id, name), a, b);
	}
	void uniform(const char *name, float a, float b, float c) {
		glUniform3f(glGetUniformLocation(_shader_id, name), a, b, c);
	}
	void uniform(const char *name, float a, float b, float c, float d) {
		glUniform4f(glGetUniformLocation(_shader_id, name), a, b, c, d);
	}

	void uniform(const char *name, float *values, unsigned int length) {
		auto location = glGetUniformLocation(_shader_id, name);

		switch (length)
		{
		case 1:
			glUniform1fv(location, length, values);
			break;
		case 2:
			glUniform2fv(location, length, values);
			break;
		case 3:
			glUniform3fv(location, length, values);
			break;
		case 4:
			glUniform4fv(location, length, values);
			break;
		default:
			break;
		}
	}

private:
	unsigned int _shader_id;
};

#endif