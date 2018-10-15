#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include <iostream>
using namespace std;

#pragma comment(lib, "glfw3dll.lib")

/*
* Modern OpenGL
*
* VAO - Vertex Array Object 顶点数组对象
* - VAO是对顶点结构的抽象描述，用以告诉渲染管线（GPU）哪部分是位置？
*	哪部分是颜色？
*
* VBO - Vertex Buffer Object 顶点缓存对象
* -	OpenGL借助VBO可以一次性将大量顶点交付给渲染管线
*
* 绑定和解绑
* (1) 绑定完数据后解绑VAO
* (2) 绑定玩数据后解绑VBO
* (3) 不要随意解绑IBO
*
* 流程
* glGenVertexArrays(count, id) 创建1/多个Vertex Array
* glBindVertexArray(id) 指定当前使用的Vertex Array
* 
* glGenBuffers(count, id) 创建1/多个Buffer Object
* glBindBuffer(buffer_type, id) 指定当前使用的Buffer
* glBufferData(buffer_type, size, data, draw_type) 传输数据
* 
* glVertexAttribPointer(index, size, type, normalized, stride, pointer)
* glEnableVertexAttribArray
*
* glBindVertexArray
*
*/

// 窗口大小调整回调函数
// --------------------
void framebuffer_size_callback(GLFWwindow *window, int width, int height);

// 输入处理函数
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

	// 现代OpenGL需要至少设置一个顶点和一个片段着色器
	// location : 输入变量的位置值

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
	// 附加着色器源码到着色器对象
	glShaderSource(vertex_shader, 1, &vertex_shader_source, NULL);
	// 编译着色器
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

	// 着色器程序对象
	// --------------
	unsigned int shader_program = glCreateProgram();
	// 附加着色器
	glAttachShader(shader_program, vertex_shader);
	glAttachShader(shader_program, fragment_shader);
	// 链接成最终对象
	glLinkProgram(shader_program);

	// 删除着色器对象
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

	// 顶点数组对象
	// 用于记录绘制时的顶点数据与顶点设置 (VertexArrayPointer & Element Array)
	// VAO会记录EBO的解绑调用，所以在解绑VAO前不要随意解绑EBO
	// -----------
	unsigned int VAO;
	glGenVertexArrays(1, &VAO);

	unsigned int VBO;
	glGenBuffers(1, &VBO);

	unsigned int EBO;
	glGenBuffers(1, &EBO);

	// 绑定VAO对象
	glBindVertexArray(VAO);

	// OpenGL允许同时绑定多个缓冲，只要是不同的缓冲类型
	// 绑定后再使用任何再GL_ARRAY_BUFFER上的缓冲调用都会配置当前绑定的缓冲
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	// 将内存中的顶点数据复制到显存中
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	// 绑定EBO
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	// 顶点着色器允许指定任何以顶点属性为形式的输入
	// 所以必须手动指定输入数据的哪一部分对应着着色器的哪一个属性
	// ----------------------------------------------------
	// 调用时使用的VBO是当前绑定的VBO
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	// 参数1: 对应着色器中的位置(location)值
	// 参数2: 指定顶点属性的维度
	// 参数3: 指定数据类型
	// 参数4: 是否标准化（映射到-1~1）
	// 参数5: 连续顶点属性的间隔
	// 参数6: 位置数据在缓冲中起始位置的偏移量

	// 激活VAO
	glEnableVertexAttribArray(0);

	// 解绑VBO
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	// 解绑VAO
	glBindVertexArray(0);

	// 线框模式
	//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	// 填充模式
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	while (!glfwWindowShouldClose(window))
	{
		// 处理输入
		process_input(window);

		// 渲染指令
		// ---------------
		glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		// 使用程序对象
		glUseProgram(shader_program);
		glBindVertexArray(VAO);
		//glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
		//glDrawArrays(GL_TRIANGLES, 0, 3);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
		glBindVertexArray(0);
		// 参数2: 顶点数组起始索引
		// 参数3: 绘制顶点个数

		// 检查是否有触发事件
		glfwPollEvents();
		// 交换颜色缓冲（双缓冲）
		glfwSwapBuffers(window);
	}

	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);

	glfwTerminate();
	return 0;
}

void framebuffer_size_callback(GLFWwindow *window, int width, int height)
{
	// OpenGL使用glViewport中定义的位置和宽高进行2D坐标转换
	glViewport(0, 0, width, height);
}

void process_input(GLFWwindow *window)
{
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
	{
		glfwSetWindowShouldClose(window, true);
	}
}