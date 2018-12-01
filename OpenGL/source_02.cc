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

const unsigned int SRC_WIDTH = 800;
const unsigned int SRC_HEIGHT = 600;

static CCamera camera(vec3(0.0f, 0.0f, 3.0f));
static float last_x = SRC_WIDTH / 2.0f;
static float last_y = SRC_HEIGHT / 2.0f;
static bool first_mouse = true;

static float curtFrame = 0;
static float dltaFrame = 0;
static float lastFrame = 0;

static vec3 light_pos(1.2f, 1.0f, 2.0f);

static void process_keybord_input(GLFWwindow *window)
{
	if (GLFW_PRESS == glfwGetKey(window, GLFW_KEY_ESCAPE))
	{
		glfwSetWindowShouldClose(window, true);
	}

	if (GLFW_PRESS == glfwGetKey(window, GLFW_KEY_W))
	{
		camera.on_keyboard_input(FORWARD, dltaFrame);
	}
	if (GLFW_PRESS == glfwGetKey(window, GLFW_KEY_S))
	{
		camera.on_keyboard_input(BACKWARD, dltaFrame);
	}
	if (GLFW_PRESS == glfwGetKey(window, GLFW_KEY_A))
	{
		camera.on_keyboard_input(LEFT, dltaFrame);
	}
	if (GLFW_PRESS == glfwGetKey(window, GLFW_KEY_D))
	{
		camera.on_keyboard_input(RIGHT, dltaFrame);
	}
}

static void framebuffer_size_callback(GLFWwindow *window, int width, int height)
{
	glViewport(0, 0, width, height);
}

static void process_mouse_callback(GLFWwindow *window, double xpos, double ypos)
{
	if (first_mouse)
	{
		last_x = xpos;
		last_y = ypos;
		first_mouse = false;
	}

	camera.on_mouse_move(xpos - last_x, last_y - ypos);
	last_x = xpos;
	last_y = ypos;
}

static void process_mouse_scroll(GLFWwindow *window, double offset_x, double offset_y)
{
	camera.on_mouse_scroll(offset_y);
}

int __main_02(int argc, char *argv[])
{
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	GLFWwindow *window = glfwCreateWindow(SRC_WIDTH, SRC_HEIGHT, "LearnOpenGL", NULL, NULL);
	if (NULL == window)
	{
		cout << "Failed to create GLFW window" << endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetCursorPosCallback(window, process_mouse_callback);
	glfwSetScrollCallback(window, process_mouse_scroll);

	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		cout << "Failed to initialize GLAD" << endl;
		return -1;
	}

	// enable depth
	glEnable(GL_DEPTH_TEST);

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

	unsigned int vbo, vaoLight, vaoCube;
	glGenVertexArrays(1, &vaoCube);
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	// set cube vertex array
	glBindVertexArray(vaoCube);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);
	glBindVertexArray(0);

	// set light vertex array
	glGenVertexArrays(1, &vaoLight);	// create
	glBindVertexArray(vaoLight);		// bind, change context

	// why need to bind vbo again ???
	// init bind -> set data -> set first va -> bind -> set second va ???
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);
	glEnableVertexAttribArray(0);

	// create cube's shader and lamp's shader
	CShader shaderCube("shaders/02-01-cube.vert", "shaders/02-01-cube.frag");
	CShader shaderLamp("shaders/02-01-cube.vert", "shaders/02-01-cube.frag");

	// here is the render loop
	while (!glfwWindowShouldClose(window))
	{
		curtFrame = glfwGetTime();
		dltaFrame = curtFrame - lastFrame;
		lastFrame = curtFrame;

		process_keybord_input(window);

		glClearColor(.1f, .1f, .1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		shaderCube.use();
		shaderCube.uniform("objectColor", 1.0f, 0.5f, 0.31f);
		shaderCube.uniform("lightColor",  1.0f, 1.0f, 1.0f);

		mat4 projection = perspective(radians(camera.zoom()), float(SRC_WIDTH) / float(SRC_HEIGHT), 0.1f, 100.0f);
		mat4 view = camera.get_view_matrix();
		shaderCube.uniform("projection", value_ptr(projection), 4);
		shaderCube.uniform("view", value_ptr(view), 4);
		mat4 model = mat4();
		model = translate(model, light_pos);
		model = scale(model, vec3(0.2f));
		shaderLamp.uniform("model", value_ptr(model), 4);

		glBindVertexArray(vaoLight);
		glDrawArrays(GL_TRIANGLES, 0, 36);

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	glDeleteVertexArrays(1, &vaoCube);
	glDeleteVertexArrays(1, &vaoLight);
	glDeleteBuffers(1, &vbo);

	glfwTerminate();
	return 0;
}