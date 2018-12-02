#ifndef __C_SHADER_H__
#define __C_SHADER_H__

class CShader
{
public:
	static unsigned int glCreateShaderByFile(const char *, GLenum);
	static unsigned int glCreateShaderByString(const char *, GLenum);

	CShader(const char *vertexShaderPath, const char *fragmentShaderPath);
	~CShader() = default;

	void use() {
		glUseProgram(_shaderId);
	}
	
	// 工具函数：设置uniform参数值
	void uniform(const char *name, bool value) {
		glUniform1i(glGetUniformLocation(_shaderId, name), int(value));
	}

	void uniform(const char *name, int value) {
		glUniform1i(glGetUniformLocation(_shaderId, name), value);
	}

	void uniform(const char *name, float a) {
		glUniform1f(glGetUniformLocation(_shaderId, name), a);
	}
	void uniform(const char *name, float a, float b) {
		glUniform2f(glGetUniformLocation(_shaderId, name), a, b);
	}
	void uniform(const char *name, float a, float b, float c) {
		glUniform3f(glGetUniformLocation(_shaderId, name), a, b, c);
	}
	void uniform(const char *name, float a, float b, float c, float d) {
		glUniform4f(glGetUniformLocation(_shaderId, name), a, b, c, d);
	}

	void uniformVec(const char *name, float *values, unsigned int length) {
		auto location = glGetUniformLocation(_shaderId, name);

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

	void uniformMat(const char *name, float *values, unsigned int length) {
		auto location = glGetUniformLocation(_shaderId, name);

		switch (length)
		{
		case 2:
			glUniformMatrix2fv(location, 1, GL_FALSE, values);
			break;
		case 3:
			glUniformMatrix3fv(location, 1, GL_FALSE, values);
			break;
		case 4:
			glUniformMatrix4fv(location, 1, GL_FALSE, values);
			break;
		default:
			break;
		}
	}

private:
	unsigned int _shaderId;
};

#endif