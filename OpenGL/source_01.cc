#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <tbox/tbox.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "Shader.h"
#include "stb_image.h"

#include <iostream>
#include <fstream>
#include <string>

using namespace std;

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

// camera = projection * veiw * model * local
// 矩阵运算的顺序是相反的

// 窗口大小调整回调函数
// --------------------
static void framebuffer_size_callback(GLFWwindow *window, int width, int height);

// 输入处理函数
static void process_keybord_input(GLFWwindow *window);
static void process_mouse_callback(GLFWwindow *, double, double);
static void process_mouse_scroll(GLFWwindow *window, double offset_x, double offset_y);

// 工具函数
// ------------------
unsigned int create_shader_by_file(const char *fname, GLenum type);
unsigned int create_shader_by_string(const char *shader_source, GLenum type);
unsigned int create_texture_by_file(const char *fname);

glm::vec3 camera_pos = glm::vec3(0.0f, 0.0f, 3.0f);
glm::vec3 camera_front = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 camera_up = glm::vec3(0.0f, 1.0f, 0.0f);

float yaw, pitch;
float fov = 45.0f;
int __main_01(int argc, char *argv[])
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
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	glfwSetCursorPosCallback(window, process_mouse_callback);
	glfwSetScrollCallback(window, process_mouse_scroll);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		cout << "Failed to initialize GLAD" << endl;
		return -1;
	}

	glEnable(GL_DEPTH_TEST);
	//stbi_set_flip_vertically_on_load(true);

	float vertices[] = {
	-0.5f, -0.5f, -0.5f,  0.0f, 0.0f,
	 0.5f, -0.5f, -0.5f,  1.0f, 0.0f,
	 0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
	 0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
	-0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
	-0.5f, -0.5f, -0.5f,  0.0f, 0.0f,

	-0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
	 0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
	 0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
	 0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
	-0.5f,  0.5f,  0.5f,  0.0f, 1.0f,
	-0.5f, -0.5f,  0.5f,  0.0f, 0.0f,

	-0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
	-0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
	-0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
	-0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
	-0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
	-0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

	 0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
	 0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
	 0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
	 0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
	 0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
	 0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

	-0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
	 0.5f, -0.5f, -0.5f,  1.0f, 1.0f,
	 0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
	 0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
	-0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
	-0.5f, -0.5f, -0.5f,  0.0f, 1.0f,

	-0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
	 0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
	 0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
	 0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
	-0.5f,  0.5f,  0.5f,  0.0f, 0.0f,
	-0.5f,  0.5f, -0.5f,  0.0f, 1.0f
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
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);	// 坐标
	glEnableVertexAttribArray(0);	// 激活

	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);

	// 参数1: 对应着色器中的位置(location)值
	// 参数2: 指定顶点属性的维度
	// 参数3: 指定数据类型
	// 参数4: 是否标准化（映射到-1~1）
	// 参数5: 连续顶点属性的间隔
	// 参数6: 位置数据在缓冲中起始位置的偏移量

	// 解绑VBO
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	// 解绑VAO
	glBindVertexArray(0);

	// 线框模式
	//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	// 填充模式
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	auto texture1 = create_texture_by_file(".\\container.jpg");
	auto texture2 = create_texture_by_file(".\\awesomeface.png");

	CShader shader_program(".\\shaders\\01-08.vert", ".\\shaders\\01-08.frag");
	shader_program.use();
	shader_program.uniform("texture1", 0);
	shader_program.uniform("texture2", 1);

//glm::mat4 view(1.0f);
//view = glm::translate(view, glm::vec3(0.0f, 0.0f, -3.0f));
//shader_program.uniform("view", glm::value_ptr(view), 4);

glm::vec3 cube_positions[] = {
	glm::vec3(0.0f,  0.0f,  0.0f),
	glm::vec3(2.0f,  5.0f, -15.0f),
	glm::vec3(-1.5f, -2.2f, -2.5f),
	glm::vec3(-3.8f, -2.0f, -12.3f),
	glm::vec3(2.4f, -0.4f, -3.5f),
	glm::vec3(-1.7f,  3.0f, -7.5f),
	glm::vec3(1.3f, -2.0f, -2.5f),
	glm::vec3(1.5f,  2.0f, -2.5f),
	glm::vec3(1.5f,  0.2f, -1.5f),
	glm::vec3(-1.3f,  1.0f, -1.5f)
};

while (!glfwWindowShouldClose(window))
{
	// 处理输入
	process_keybord_input(window);

	// 渲染指令
	// ---------------
	glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// 绑定纹理
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture1);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, texture2);

	// 使用(激活)着色器程序对象
	shader_program.use();

	glm::mat4 projection;
	projection = glm::perspective(glm::radians(fov), 800.0f / 600.0f, 0.1f, 100.0f);
	shader_program.uniformMat("projection", glm::value_ptr(projection), 4);

	glm::mat4 view(1.0f);
	view = glm::lookAt(camera_pos, camera_pos + camera_front, camera_up);
	shader_program.uniformMat("view", glm::value_ptr(view), 4);

	glBindVertexArray(VAO);
	for (auto i = 0; i < 10; ++i)
	{
		glm::mat4 model(1.0f);
		model = glm::translate(model, cube_positions[i]);
		float angle = 20.0f * i;
		model = glm::rotate(model, glm::radians(angle), glm::vec3(1.0f, 0.3f, 0.5f));
		shader_program.uniformMat("model", glm::value_ptr(model), 4);

		glDrawArrays(GL_TRIANGLES, 0, 36);
	}
	//glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	//glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
	//glDrawArrays(GL_TRIANGLES, 0, 36);

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

static float last_time = 0.0f;
void process_keybord_input(GLFWwindow *window)
{
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
	{
		glfwSetWindowShouldClose(window, true);
		return;
	}

	float current_time = glfwGetTime();
	float camera_speed = (current_time - last_time) * 2.5f;
	last_time = current_time;

	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
	{
		camera_pos += camera_speed * camera_front;
	}
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
	{
		camera_pos -= camera_speed * camera_front;
	}
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
	{
		camera_pos -= glm::normalize(glm::cross(camera_front, camera_up)) * camera_speed;
	}
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
	{
		camera_pos += glm::normalize(glm::cross(camera_front, camera_up)) * camera_speed;
	}
}

float last_x, last_y;
bool first_mouse = true;
void process_mouse_callback(GLFWwindow *window, double x, double y)
{
	if (first_mouse)
	{
		last_x = x;
		last_y = y;
		first_mouse = false;
	}

	float offset_x = x - last_x;
	float offset_y = last_y - y;
	last_x = x;
	last_y = y;

	float sensitivity = 0.05f;
	offset_x *= sensitivity;
	offset_y *= sensitivity;

	yaw   += offset_x;
	pitch += offset_y;

	if (pitch > 89.0f)
	{
		pitch = 89.0f;
	}
	if (pitch < -89.0f)
	{
		pitch = 89.0f;
	}

	glm::vec3 front;
	front.x = cos(glm::radians(pitch)) * cos(glm::radians(yaw));
	front.y = sin(glm::radians(pitch));
	front.z = cos(glm::radians(pitch)) * sin(glm::radians(yaw));
	camera_front = glm::normalize(front);
}

void process_mouse_scroll(GLFWwindow *window, double offset_x, double offset_y)
{
	fov -= offset_y;
	if (fov <= 1.0f)
	{
		fov = 1.0f;
	}
	if (fov >= 45.0f)
	{
		fov = 45.0f;
	}

	cout << fov << endl;
}

unsigned int create_shader_by_string(const char *shader_source, GLenum type)
{
	// 创建Shader
	unsigned int shader = glCreateShader(type);
	// 设置源码
	glShaderSource(shader, 1, &shader_source, NULL);
	// 编译Shader
	glCompileShader(shader);

	// 检查错误
	int success;
	char log_buffer[512];
	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(shader, 512, NULL, log_buffer);
		cout << "ERROR::SHADER::" << type << "::COMPILATION_FAILED\n" << log_buffer << endl;
		return 0;
	}

	return shader;
}

unsigned int create_shader_by_file(const char *fname, GLenum type)
{
	auto shader_file = ifstream(fname, ios::in);

	istreambuf_iterator<char> beg(shader_file), end;

	string content(beg, end);

	return create_shader_by_string(content.c_str(), type);
}

unsigned int create_texture_by_file(const char *fname)
{
	unsigned int texture;
	glGenTextures(1, &texture);

	glBindTexture(GL_TEXTURE_2D, texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	int width, height, channels;
	unsigned char *image_data = stbi_load(fname, &width, &height, &channels, 0);
	if (image_data)
	{
		// 参数1: 指定纹理目标（当前绑定到GL_TEXTURE_2D上的对象）
		// 参数2: 指定多级渐远纹理的级别
		// 参数3: 将纹理储存为何种格式
		// 参数4: 纹理宽度
		// 参数5: 纹理高度
		// 参数6: 恒为0
		// 参数7: 源图片格式
		// 参数8: 源图片数据类型
		// 参数9: 图像数据
		if (strstr(fname, ".png"))
		{
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image_data);
		}
		else {
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image_data);
		}
		
		// 自动生成多级渐远纹理
		glGenerateMipmap(GL_TEXTURE_2D);
	}
	else 
	{
		cout << "Failed to load texture::" << fname << endl;
		texture = 0;
	}

	stbi_image_free(image_data);
	glBindTexture(GL_TEXTURE_2D, 0);
	return texture;
}