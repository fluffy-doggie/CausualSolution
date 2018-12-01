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

// camera = projection * veiw * model * local
// ���������˳�����෴��

// ���ڴ�С�����ص�����
// --------------------
static void framebuffer_size_callback(GLFWwindow *window, int width, int height);

// ���봦����
static void process_keybord_input(GLFWwindow *window);
static void process_mouse_callback(GLFWwindow *, double, double);
static void process_mouse_scroll(GLFWwindow *window, double offset_x, double offset_y);

// ���ߺ���
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
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);	// ����
	glEnableVertexAttribArray(0);	// ����

	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);

	// ����1: ��Ӧ��ɫ���е�λ��(location)ֵ
	// ����2: ָ���������Ե�ά��
	// ����3: ָ����������
	// ����4: �Ƿ��׼����ӳ�䵽-1~1��
	// ����5: �����������Եļ��
	// ����6: λ�������ڻ�������ʼλ�õ�ƫ����

	// ���VBO
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	// ���VAO
	glBindVertexArray(0);

	// �߿�ģʽ
	//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	// ���ģʽ
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
	// ��������
	process_keybord_input(window);

	// ��Ⱦָ��
	// ---------------
	glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// ������
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture1);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, texture2);

	// ʹ��(����)��ɫ���������
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
	// ����Shader
	unsigned int shader = glCreateShader(type);
	// ����Դ��
	glShaderSource(shader, 1, &shader_source, NULL);
	// ����Shader
	glCompileShader(shader);

	// ������
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
		// ����1: ָ������Ŀ�꣨��ǰ�󶨵�GL_TEXTURE_2D�ϵĶ���
		// ����2: ָ���༶��Զ����ļ���
		// ����3: ��������Ϊ���ָ�ʽ
		// ����4: ������
		// ����5: ����߶�
		// ����6: ��Ϊ0
		// ����7: ԴͼƬ��ʽ
		// ����8: ԴͼƬ��������
		// ����9: ͼ������
		if (strstr(fname, ".png"))
		{
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image_data);
		}
		else {
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image_data);
		}
		
		// �Զ����ɶ༶��Զ����
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