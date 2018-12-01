#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "Shader.h"
#include "Camera.h"
#include "stb_image.h"

#include <iostream>
#include <string>

using namespace std;
using namespace glm;

void frameBufferSizeCallback(GLFWwindow *window, int width, int height);
void mouseCallback(GLFWwindow *window, double xpos, double ypos);
void scrollCallback(GLFWwindow *window, double xoffset, double yoffset);
void processInput(GLFWwindow *window);

const unsigned SCR_WIDTH = 800;
const unsigned SCR_HEIGHT = 600;

CCamera camera(vec3(0.0f, 0.0f, 3.0f));
float lastX = SCR_WIDTH  / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

float dltaTime = 0.0f;
float lastTime = 0.0f;

vec3 lampPos(1.2f, 1.0f, 2.0f);

int __main_02(int argc, char *argv[])
{
	// glfw init logic
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // uncomment this statement to fix compilation on OS X
#endif

	// create a window
	GLFWwindow *window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
	if (NULL == window)
	{
		cout << "Failed to create GLFW window" << endl;
		glfwTerminate();
		return -1;
	}

	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, frameBufferSizeCallback);
	glfwSetCursorPosCallback(window, mouseCallback);
	glfwSetScrollCallback(window, scrollCallback);

	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		cout << "Failed to initialize GLAD" << endl;
		return -1;
	}

	// open depth test
	glEnable(GL_DEPTH_TEST);

	CShader cubeShader("shaders/02-01-cube.vert", "shaders/02-01-cube.frag");
	CShader lampShader("shaders/02-01-lamp.vert", "shaders/02-01-lamp.frag");

	// vertices data
	float vertices[] = {
		-0.5f, -0.5f, -0.5f,
		 0.5f, -0.5f, -0.5f,
		 0.5f,  0.5f, -0.5f,
		 0.5f,  0.5f, -0.5f,
		-0.5f,  0.5f, -0.5f,
		-0.5f, -0.5f, -0.5f,

		-0.5f, -0.5f,  0.5f,
		 0.5f, -0.5f,  0.5f,
		 0.5f,  0.5f,  0.5f,
		 0.5f,  0.5f,  0.5f,
		-0.5f,  0.5f,  0.5f,
		-0.5f, -0.5f,  0.5f,

		-0.5f,  0.5f,  0.5f,
		-0.5f,  0.5f, -0.5f,
		-0.5f, -0.5f, -0.5f,
		-0.5f, -0.5f, -0.5f,
		-0.5f, -0.5f,  0.5f,
		-0.5f,  0.5f,  0.5f,

		 0.5f,  0.5f,  0.5f,
		 0.5f,  0.5f, -0.5f,
		 0.5f, -0.5f, -0.5f,
		 0.5f, -0.5f, -0.5f,
		 0.5f, -0.5f,  0.5f,
		 0.5f,  0.5f,  0.5f,

		-0.5f, -0.5f, -0.5f,
		 0.5f, -0.5f, -0.5f,
		 0.5f, -0.5f,  0.5f,
		 0.5f, -0.5f,  0.5f,
		-0.5f, -0.5f,  0.5f,
		-0.5f, -0.5f, -0.5f,

		-0.5f,  0.5f, -0.5f,
		 0.5f,  0.5f, -0.5f,
		 0.5f,  0.5f,  0.5f,
		 0.5f,  0.5f,  0.5f,
		-0.5f,  0.5f,  0.5f,
		-0.5f,  0.5f, -0.5f,
	};

	// vertex buffer object
	// vertex array object cube/lamp
	unsigned vbo, vaoCube, vaoLamp;
	glGenVertexArrays(1, &vaoCube);
	glGenBuffers(1, &vbo);

	// move cpu ram data to gpu ram through vbo
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
	
	glBindVertexArray(vaoCube);

	// set va datas structure (cube)
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);
	glEnableVertexAttribArray(0);

	glGenVertexArrays(1, &vaoLamp);
	glBindVertexArray(vaoLamp);

	glBindBuffer(GL_ARRAY_BUFFER, vbo);

	// set va datas structure (lamp)
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);
	glEnableVertexAttribArray(0);

	// start render loop
	while (!glfwWindowShouldClose(window))
	{
		float curtTime = glfwGetTime();
		dltaTime = curtTime - lastTime;
		lastTime = curtTime;

		processInput(window);

		glClearColor(.2f, .2f, .2f, .2f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		cubeShader.use();
		cubeShader.uniform("objectColor", 1.0f, 0.5f, 0.31f);
		cubeShader.uniform("lightColor", 1.0f, 1.0f, 1.0f);

		mat4 projection = perspective(radians(camera.zoom()), float(SCR_WIDTH) / float(SCR_HEIGHT), 0.1f, 100.0f);
		mat4 view = camera.get_view_matrix();
		mat4 model(1.0f);
		cubeShader.uniformMat("projection", value_ptr(projection), 4);
		cubeShader.uniformMat("view", value_ptr(view), 4);
		cubeShader.uniformMat("model", value_ptr(model), 4);

		glBindVertexArray(vaoCube);
		glDrawArrays(GL_TRIANGLES, 0, 36);

		// draw the lamp object
		lampShader.use();
		lampShader.uniformMat("projection", value_ptr(projection), 4);
		lampShader.uniformMat("view", value_ptr(view), 4);
		model = mat4(1.0f);
		model = translate(model, lampPos);
		model = scale(model, vec3(0.2f));
		lampShader.uniformMat("model", value_ptr(model), 4);

		glBindVertexArray(vaoLamp);
		glDrawArrays(GL_TRIANGLES, 0, 36);

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	glDeleteVertexArrays(1, &vaoCube);
	glDeleteVertexArrays(1, &vaoLamp);
	glDeleteBuffers(1, &vbo);

	glfwTerminate();
	return 0;
}

void processInput(GLFWwindow *window)
{
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);

	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		camera.on_keyboard_input(FORWARD, dltaTime);
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		camera.on_keyboard_input(BACKWARD, dltaTime);
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		camera.on_keyboard_input(LEFT, dltaTime);
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		camera.on_keyboard_input(RIGHT, dltaTime);
}

void frameBufferSizeCallback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
}

void mouseCallback(GLFWwindow* window, double xpos, double ypos)
{
	if (firstMouse)
	{
		lastX = xpos;
		lastY = ypos;
		firstMouse = false;
	}

	float xoffset = xpos - lastX;
	float yoffset = lastY - ypos;

	lastX = xpos;
	lastY = ypos;

	camera.on_mouse_move(xoffset, yoffset);
}

void scrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
	camera.on_mouse_scroll(yoffset);
}