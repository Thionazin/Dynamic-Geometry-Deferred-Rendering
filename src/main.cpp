#include <cassert>
#include <cstring>
#define _USE_MATH_DEFINES
#include <cmath>
#include <iostream>

#include <random>

#include <cstdlib>
#include <ctime>

#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include "Camera.h"
#include "GLSL.h"
#include "MatrixStack.h"
#include "Program.h"
#include "Shape.h"
#include "Texture.h"

#include "WorldObject.h"
#include "Light.h"

using namespace std;

GLFWwindow *window; // Main application window
string RESOURCE_DIR = "./"; // Where the resources are loaded from
bool OFFLINE = false;

shared_ptr<Camera> camera;
shared_ptr<Program> prog;

shared_ptr<Shape> shape;
shared_ptr<Shape> teapot;
shared_ptr<Shape> w_floor;
shared_ptr<Shape> sphere;

vector<glm::vec3> light_positions;
vector<glm::vec3> light_colors;

vector<WorldObject> wobjs;

bool keyToggles[256] = {false}; // only for English keyboards!

// This function is called when a GLFW error occurs
static void error_callback(int error, const char *description)
{
	cerr << description << endl;
}

// This function is called when a key is pressed
static void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
	if(key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
		glfwSetWindowShouldClose(window, GL_TRUE);
	}
}

// This function is called when the mouse is clicked
static void mouse_button_callback(GLFWwindow *window, int button, int action, int mods)
{
	// Get the current mouse position.
	double xmouse, ymouse;
	glfwGetCursorPos(window, &xmouse, &ymouse);
	// Get current window size.
	int width, height;
	glfwGetWindowSize(window, &width, &height);
	if(action == GLFW_PRESS) {
		bool shift = (mods & GLFW_MOD_SHIFT) != 0;
		bool ctrl  = (mods & GLFW_MOD_CONTROL) != 0;
		bool alt   = (mods & GLFW_MOD_ALT) != 0;
		camera->mouseClicked((float)xmouse, (float)ymouse, shift, ctrl, alt);
	}
}

// This function is called when the mouse moves
static void cursor_position_callback(GLFWwindow* window, double xmouse, double ymouse)
{
	int state = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);
	if(state == GLFW_PRESS) {
		camera->mouseMoved((float)xmouse, (float)ymouse);
	}
}

static void char_callback(GLFWwindow *window, unsigned int key)
{
	keyToggles[key] = !keyToggles[key];
}

// If the window is resized, capture the new size and reset the viewport
static void resize_callback(GLFWwindow *window, int width, int height)
{
	glViewport(0, 0, width, height);
}

// https://lencerf.github.io/post/2019-09-21-save-the-opengl-rendering-to-image-file/
static void saveImage(const char *filepath, GLFWwindow *w)
{
	int width, height;
	glfwGetFramebufferSize(w, &width, &height);
	GLsizei nrChannels = 3;
	GLsizei stride = nrChannels * width;
	stride += (stride % 4) ? (4 - stride % 4) : 0;
	GLsizei bufferSize = stride * height;
	std::vector<char> buffer(bufferSize);
	glPixelStorei(GL_PACK_ALIGNMENT, 4);
	glReadBuffer(GL_BACK);
	glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, buffer.data());
	stbi_flip_vertically_on_write(true);
	int rc = stbi_write_png(filepath, width, height, nrChannels, buffer.data(), stride);
	if(rc) {
		cout << "Wrote to " << filepath << endl;
	} else {
		cout << "Couldn't write to " << filepath << endl;
	}
}

// This function is called once to initialize the scene and OpenGL
static void init()
{
	// Initialize time.
	glfwSetTime(0.0);
	
	// Set background color.
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	// Enable z-buffer test.
	glEnable(GL_DEPTH_TEST);


	//initialize the shaders
	prog = make_shared<Program>();
	prog->setShaderNames(RESOURCE_DIR + "bp_vert.glsl", RESOURCE_DIR + "bp_frag.glsl");
	prog->setVerbose(true);
	prog->init();
	prog->addAttribute("aPos");
	prog->addAttribute("aNor");
	prog->addUniform("MV");
	prog->addUniform("P");
	prog->addUniform("IT");
	prog->addUniform("light_positions");
	prog->addUniform("light_colors");
	prog->addUniform("ka");
	prog->addUniform("kd");
	prog->addUniform("ks");
	prog->addUniform("s");
	prog->setVerbose(false);

	camera = make_shared<Camera>();
	camera->setInitDistance(20.0f); // Camera's initial Z translation
	
	shape = make_shared<Shape>();
	shape->loadMesh(RESOURCE_DIR + "bunny.obj");
	shape->init();

	teapot = make_shared<Shape>();
	teapot->loadMesh(RESOURCE_DIR + "teapot.obj");
	teapot->init();

	w_floor = make_shared<Shape>();
	w_floor->loadMesh(RESOURCE_DIR + "square.obj");
	w_floor->init();

	sphere = make_shared<Shape>();
	sphere->loadMesh(RESOURCE_DIR + "sphere.obj");
	sphere->init();

	std::random_device randevice;
	std::mt19937 gen(randevice());
	std::uniform_real_distribution<> distr(0.2, 0.6);
	
	// Add each individual world object
	for(int i = 0; i < 10; i++) {
		for(int j = 0; j < 10; j++) {
			glm::vec3 rotation(0.0, 0.0, 0.0);
			glm::vec3 translation(i, 0.0, j);
			double scale_val = distr(gen);
			glm::vec3 scale(scale_val, scale_val, scale_val);
			glm::vec3 ambient(0.0, 0.0, 0.0);
			glm::vec3 diffuse(((double) std::rand() / (RAND_MAX)), ((double) std::rand() / (RAND_MAX)), ((double) std::rand() / (RAND_MAX)));
			glm::vec3 specular(1.0f, 1.0f, 1.0f);
			double shininess = 10.0;
			if((i + j) % 2 == 0) {
				wobjs.emplace_back(rotation, translation, scale, shape, ambient, diffuse, specular, shininess);
			} else {
				wobjs.emplace_back(rotation, translation, scale, teapot, ambient, diffuse, specular, shininess);
			}
		}
	}

	// Add the lights
	// Hard code for now
	/*
	for(int i = 0; i < 3; i++) {
		glm::vec3 l_pos(i, 2.0, j);
		glm::vec3 l_col();
	}
	*/
	{
		light_positions.emplace_back(1.5, 0.2, 1.5);
		light_colors.emplace_back(1.0, 1.0, 1.0);
	}

	{
		light_positions.emplace_back(2.5, 0.2, 2.5);
		light_colors.emplace_back(0.0, 1.0, 0.0);
	}
	
	{
		light_positions.emplace_back(3.5, 0.2, 3.5);
		light_colors.emplace_back(0.0, 0.0, 1.0);
	}

	// Add the sun
	{
		glm::vec3 rotation(0.0, 0.0, 0.0);
		glm::vec3 translation(10.0, 10.0, 10.0);
		glm::vec3 scale(1.0, 1.0, 1.0);
		glm::vec3 ambient(1.0, 1.0, 0.0);
		glm::vec3 diffuse(0.0, 0.0, 0.0);
		glm::vec3 specular(0.0, 0.0, 0.0);
		double shininess = 1;
		wobjs.emplace_back(rotation, translation, scale, sphere, ambient, diffuse, specular, shininess);
	}
	

	// Add the floor
	{
		glm::vec3 rotation(0.0, 0.0, 0.0);
		glm::vec3 translation(4.5, 0.0, 4.5);
		glm::vec3 scale(12.0, 1.0, 12.0);
		glm::vec3 ambient(0.0, 0.0, 0.0);
		glm::vec3 diffuse(1.0, 1.0, 1.0);
		glm::vec3 specular(1.0, 1.0, 1.0);
		double shininess = 10;
		wobjs.emplace_back(rotation, translation, scale, w_floor, ambient, diffuse, specular, shininess);
	}
	
	GLSL::checkError(GET_FILE_LINE);
}

// This function is called every frame to draw the scene.
static void render()
{
	// Clear framebuffer.
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	if(keyToggles[(unsigned)'c']) {
		glEnable(GL_CULL_FACE);
	} else {
		glDisable(GL_CULL_FACE);
	}

	// Get current frame buffer size.
	int width, height;
	glfwGetFramebufferSize(window, &width, &height);
	camera->setAspect((float)width/(float)height);
	
	double t = glfwGetTime();

	glViewport(0, 0, width, height);
	
	// Matrix stacks
	auto P = make_shared<MatrixStack>();
	auto MV = make_shared<MatrixStack>();
	
	// Apply camera transforms
	P->pushMatrix();
	camera->applyProjectionMatrix(P);
	MV->pushMatrix();
	camera->applyViewMatrix(MV);


	// Handle the sun
	glm::vec3 camera_lights[3];
	glm::mat4 light_matrix = MV->topMatrix();
	for(int i = 0; i < 3; i++) {
		glm::vec4 l_pos_cord(light_positions[i], 1.0);
		camera_lights[i] = light_matrix * l_pos_cord;
	}

	
	// Make the ground
	MV->pushMatrix();
		MV->translate(wobjs[wobjs.size()-1].translate);
		MV->scale(wobjs[wobjs.size()-1].scale);
		MV->rotate(3*(M_PI/2), 1.0, 0.0, 0.0);
		prog->bind();
		glUniformMatrix4fv(prog->getUniform("P"), 1, GL_FALSE, glm::value_ptr(P->topMatrix()));
		glUniformMatrix4fv(prog->getUniform("MV"), 1, GL_FALSE, glm::value_ptr(MV->topMatrix()));
		glUniformMatrix4fv(prog->getUniform("IT"), 1, GL_FALSE, glm::value_ptr(glm::inverse(glm::transpose(MV->topMatrix()))));
		glUniform3fv(prog->getUniform("light_positions"), 3, glm::value_ptr(camera_lights[0]));
		glUniform3fv(prog->getUniform("light_colors"), 3, glm::value_ptr(light_colors.data()[0]));
		glUniform3fv(prog->getUniform("ka"), 1, glm::value_ptr(wobjs[wobjs.size()-1].ambient));
		glUniform3fv(prog->getUniform("kd"), 1, glm::value_ptr(wobjs[wobjs.size()-1].diffuse));
		glUniform3fv(prog->getUniform("ks"), 1, glm::value_ptr(wobjs[wobjs.size()-1].specular));
		glUniform1f(prog->getUniform("s"), wobjs[wobjs.size()-1].shiny);
		wobjs[wobjs.size()-1].shape->draw(prog);
		prog->unbind();
	MV->popMatrix();

	// Make the lights
	for(int i = 0; i < 3; i++) {
		MV->pushMatrix();
			MV->translate(light_positions[i]);
			MV->scale(0.1, 0.1, 0.1);
			prog->bind();
			glUniformMatrix4fv(prog->getUniform("P"), 1, GL_FALSE, glm::value_ptr(P->topMatrix()));
			glUniformMatrix4fv(prog->getUniform("MV"), 1, GL_FALSE, glm::value_ptr(MV->topMatrix()));
			glUniformMatrix4fv(prog->getUniform("IT"), 1, GL_FALSE, glm::value_ptr(glm::inverse(glm::transpose(MV->topMatrix()))));
			glUniform3fv(prog->getUniform("light_positions"), 3, glm::value_ptr(camera_lights[0]));
			glUniform3fv(prog->getUniform("light_colors"), 3, glm::value_ptr(light_colors.data()[0]));
			glUniform3fv(prog->getUniform("ka"), 1, glm::value_ptr(light_colors[i]));
			glm::vec3 zero_vec(0.0);
			glUniform3fv(prog->getUniform("kd"), 1, glm::value_ptr(zero_vec));
			glUniform3fv(prog->getUniform("ks"), 1, glm::value_ptr(zero_vec));
			glUniform1f(prog->getUniform("s"), 1);
			sphere->draw(prog);
			prog->unbind();
		MV->popMatrix();
	}
	
	// Apply all transformations
	for(unsigned int i = 0; i < wobjs.size()-2; i++) {	
		MV->pushMatrix();
			MV->translate(wobjs[i].translate);
			MV->translate(0.0, (0.0-wobjs[i].shape->lowest_y)*wobjs[i].scale.y, 0.0);
			if(wobjs[i].shape == shape) {
				MV->rotate(t, 0.0, 1.0, 0.0);
			} else if(wobjs[i].shape == teapot) {
				glm::mat4 S(1.0f);
				S[1][2] = 0.5f*cos(t);
				MV->multMatrix(S);
			}
			MV->scale(wobjs[i].scale);
			prog->bind();
			glUniformMatrix4fv(prog->getUniform("P"), 1, GL_FALSE, glm::value_ptr(P->topMatrix()));
			glUniformMatrix4fv(prog->getUniform("MV"), 1, GL_FALSE, glm::value_ptr(MV->topMatrix()));
			glUniformMatrix4fv(prog->getUniform("IT"), 1, GL_FALSE, glm::value_ptr(glm::inverse(glm::transpose(MV->topMatrix()))));
			glUniform3fv(prog->getUniform("light_positions"), 3, glm::value_ptr(camera_lights[0]));
			glUniform3fv(prog->getUniform("light_colors"), 3, glm::value_ptr(light_colors.data()[0]));
			glUniform3fv(prog->getUniform("ka"), 1, glm::value_ptr(wobjs[i].ambient));
			glUniform3fv(prog->getUniform("kd"), 1, glm::value_ptr(wobjs[i].diffuse));
			glUniform3fv(prog->getUniform("ks"), 1, glm::value_ptr(wobjs[i].specular));
			glUniform1f(prog->getUniform("s"), wobjs[i].shiny);
			wobjs[i].shape->draw(prog);
			prog->unbind();
		MV->popMatrix();
	}

	MV->popMatrix();
	P->popMatrix();


	GLSL::checkError(GET_FILE_LINE);
	
	if(OFFLINE) {
		saveImage("output.png", window);
		GLSL::checkError(GET_FILE_LINE);
		glfwSetWindowShouldClose(window, true);
	}
}

int main(int argc, char **argv)
{
	if(argc < 2) {
		cout << "Usage: A3 RESOURCE_DIR" << endl;
		return 0;
	}
	RESOURCE_DIR = argv[1] + string("/");
	
	// Optional argument
	if(argc >= 3) {
		OFFLINE = atoi(argv[2]) != 0;
	}

	// Set error callback.
	glfwSetErrorCallback(error_callback);
	// Initialize the library.
	if(!glfwInit()) {
		return -1;
	}
	// Create a windowed mode window and its OpenGL context.
	window = glfwCreateWindow(640, 480, "YOUR NAME", NULL, NULL);
	if(!window) {
		glfwTerminate();
		return -1;
	}
	// Make the window's context current.
	glfwMakeContextCurrent(window);
	// Initialize GLEW.
	glewExperimental = true;
	if(glewInit() != GLEW_OK) {
		cerr << "Failed to initialize GLEW" << endl;
		return -1;
	}
	glGetError(); // A bug in glewInit() causes an error that we can safely ignore.
	cout << "OpenGL version: " << glGetString(GL_VERSION) << endl;
	cout << "GLSL version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;
	GLSL::checkVersion();
	// Set vsync.
	glfwSwapInterval(1);
	// Set keyboard callback.
	glfwSetKeyCallback(window, key_callback);
	// Set char callback.
	glfwSetCharCallback(window, char_callback);
	// Set cursor position callback.
	glfwSetCursorPosCallback(window, cursor_position_callback);
	// Set mouse button callback.
	glfwSetMouseButtonCallback(window, mouse_button_callback);
	// Set the window resize call back.
	glfwSetFramebufferSizeCallback(window, resize_callback);
	// Initialize scene.
	init();
	// Loop until the user closes the window.
	while(!glfwWindowShouldClose(window)) {
		// Render scene.
		render();
		// Swap front and back buffers.
		glfwSwapBuffers(window);
		// Poll for and process events.
		glfwPollEvents();
	}
	// Quit program.
	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}
