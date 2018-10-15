#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include <iostream>
using namespace std;

#pragma comment(lib, "glfw3dll.lib")

/*
* Modern OpenGL
*
* VAO - Vertex Array Object �����������
* - VAO�ǶԶ���ṹ�ĳ������������Ը�����Ⱦ���ߣ�GPU���Ĳ�����λ�ã�
*	�Ĳ�������ɫ��
*
* VBO - Vertex Buffer Object ���㻺�����
* -	OpenGL����VBO����һ���Խ��������㽻������Ⱦ����
*
* �󶨺ͽ��
* (1) �������ݺ���VAO
* (2) �������ݺ���VBO
* (3) ��Ҫ������IBO
*
* ����
* glGenVertexArrays(count, id) ����1/���Vertex Array
* glBindVertexArray(id) ָ����ǰʹ�õ�Vertex Array
* 
* glGenBuffers(count, id) ����1/���Buffer Object
* glBindBuffer(buffer_type, id) ָ����ǰʹ�õ�Buffer
* glBufferData(buffer_type, size, data, draw_type) ��������
* 
* glVertexAttribPointer(index, size, type, normalized, stride, pointer)
* glEnableVertexAttribArray
*
* glBindVertexArray
*
*/

// ���ڴ�С�����ص�����
// --------------------
void framebuffer_size_callback(GLFWwindow *window, int width, int height);

// ���봦����
void process_input(GLFWwindow *window);

int main(int argc, char *argv)
{
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	GLFWwindow *window = glfwCreateWindow(800, 600, "LearnOpenGL", NULL, NULL);
	if (NULL == window)
	{
		cout << "Failed to create GLFW window" << endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		cout << "Failed to initialize GLAD" << endl;
		return -1;
	}

	// �ִ�OpenGL��Ҫ��������һ�������һ��Ƭ����ɫ��
	// location : ���������λ��ֵ

	int success; char info_log[512];

	const char *vertex_shader_source = R"(
		#version 330 core
		layout(location = 0) in vec3 aPos;

		out vec4 vertexColor;

		void main ()
		{
			gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0f);
			vertexColor = vec4(0.5f, 0.0f, 0.0f, 0.0f);
		}
	)";

	unsigned int vertex_shader = glCreateShader(GL_VERTEX_SHADER);
	// ������ɫ��Դ�뵽��ɫ������
	glShaderSource(vertex_shader, 1, &vertex_shader_source, NULL);
	// ������ɫ��
	glCompileShader(vertex_shader);
	glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(vertex_shader, 512, NULL, info_log);
		cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << info_log << endl;
	}

	const char *fragment_shader_source = R"(
		#version 330 core
		out vec4 FragColor;

		in vec4 vertexColor;

		void main ()
		{
			FragColor = vertexColor;
		}
	)";

	unsigned int fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragment_shader, 1, &fragment_shader_source, NULL);
	glCompileShader(fragment_shader);
	glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(fragment_shader, 512, NULL, info_log);
		cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << info_log << endl;
	}

	// ��ɫ���������
	// --------------
	unsigned int shader_program = glCreateProgram();
	// ������ɫ��
	glAttachShader(shader_program, vertex_shader);
	glAttachShader(shader_program, fragment_shader);
	// ���ӳ����ն���
	glLinkProgram(shader_program);

	// ɾ����ɫ������
	glDeleteShader(vertex_shader);
	glDeleteShader(fragment_shader);

	float vertices[] = {
		+0.5f, +0.5f, +0.0f,
		+0.5f, -0.5f, +0.0f,
		-0.5f, -0.5f, +0.0f,
		-0.5f, +0.5f, +0.0f,
	};

	unsigned int indices[] = {
		0, 1, 3,
		1, 2, 3,
	};

	// �����������
	// ���ڼ�¼����ʱ�Ķ��������붥������ (VertexArrayPointer & Element Array)
	// VAO���¼EBO�Ľ����ã������ڽ��VAOǰ��Ҫ������EBO
	// -----------
	unsigned int VAO;
	glGenVertexArrays(1, &VAO);

	unsigned int VBO;
	glGenBuffers(1, &VBO);

	unsigned int EBO;
	glGenBuffers(1, &EBO);

	// ��VAO����
	glBindVertexArray(VAO);

	// OpenGL����ͬʱ�󶨶�����壬ֻҪ�ǲ�ͬ�Ļ�������
	// �󶨺���ʹ���κ���GL_ARRAY_BUFFER�ϵĻ�����ö������õ�ǰ�󶨵Ļ���
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	// ���ڴ��еĶ������ݸ��Ƶ��Դ���
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	// ��EBO
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	// ������ɫ������ָ���κ��Զ�������Ϊ��ʽ������
	// ���Ա����ֶ�ָ���������ݵ���һ���ֶ�Ӧ����ɫ������һ������
	// ----------------------------------------------------
	// ����ʱʹ�õ�VBO�ǵ�ǰ�󶨵�VBO
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	// ����1: ��Ӧ��ɫ���е�λ��(location)ֵ
	// ����2: ָ���������Ե�ά��
	// ����3: ָ����������
	// ����4: �Ƿ��׼����ӳ�䵽-1~1��
	// ����5: �����������Եļ��
	// ����6: λ�������ڻ�������ʼλ�õ�ƫ����

	// ����VAO
	glEnableVertexAttribArray(0);

	// ���VBO
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	// ���VAO
	glBindVertexArray(0);

	// �߿�ģʽ
	//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	// ���ģʽ
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	while (!glfwWindowShouldClose(window))
	{
		// ��������
		process_input(window);

		// ��Ⱦָ��
		// ---------------
		glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		// ʹ�ó������
		glUseProgram(shader_program);
		glBindVertexArray(VAO);
		//glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
		//glDrawArrays(GL_TRIANGLES, 0, 3);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
		glBindVertexArray(0);
		// ����2: ����������ʼ����
		// ����3: ���ƶ������

		// ����Ƿ��д����¼�
		glfwPollEvents();
		// ������ɫ���壨˫���壩
		glfwSwapBuffers(window);
	}

	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);

	glfwTerminate();
	return 0;
}

void framebuffer_size_callback(GLFWwindow *window, int width, int height)
{
	// OpenGLʹ��glViewport�ж����λ�úͿ�߽���2D����ת��
	glViewport(0, 0, width, height);
}

void process_input(GLFWwindow *window)
{
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
	{
		glfwSetWindowShouldClose(window, true);
	}
}