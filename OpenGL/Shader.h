#ifndef __C_SHADER_H__
#define __C_SHADER_H__

class CShader
{
public:
	static unsigned int create_shader_by_file(const char *, GLenum);
	static unsigned int create_shader_by_string(const char *, GLenum);

	CShader(const char *vertex_shader_path, const char *fragment_shader_path);
	~CShader();

	void use();
	
	// 工具函数：设置uniform参数值
	void uniform(const char *name, bool value);
	void uniform(const char *name, int value);
	void uniform(const char *name, float value);

	void uniform(const char *name, float *values, unsigned int length);

private:
	unsigned int _shader_id;
};

#endif