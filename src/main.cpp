#include <cassert>
#include <cstring>
#define _USE_MATH_DEFINES
#include <cmath>
#include <iostream>

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
shared_ptr<Program> prog2;
shared_ptr<Shape> shape;
shared_ptr<Shape> teapot;
shared_ptr<Shape> w_floor;
shared_ptr<Shape> sphere;
shared_ptr<Shape> frust;

shared_ptr<Texture> ground;
shared_ptr<Texture> grounds;

Light sun;

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
	//int state = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);
	//if(state == GLFW_PRESS) {
	camera->mouseMoved((float)xmouse, (float)ymouse);
	//}
}

static void char_callback(GLFWwindow *window, unsigned int key)
{
	switch(key) {
		case 'w':
			camera->moveDir(0.1);
			break;
		case 's':
			camera->moveDir(-0.1);
			break;
		case 'a':
			camera->moveSide(-0.1);
			break;
		case 'd':
			camera->moveSide(0.1);
			break;
		case 'z':
			camera->zoom(-1);
			break;
		case 'Z':
			camera->zoom(1);
		default:
			keyToggles[key] = !keyToggles[key];
	}
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
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
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
	prog->addUniform("lightPos");
	prog->addUniform("lcol");
	prog->addUniform("ka");
	prog->addUniform("kd");
	prog->addUniform("ks");
	prog->addUniform("s");
	prog->setVerbose(false);

	prog2 = make_shared<Program>();
	prog2->setShaderNames(RESOURCE_DIR + "vert.glsl", RESOURCE_DIR + "frag.glsl");
	prog2->setVerbose(true);
	prog2->init();
	prog2->addAttribute("aPos");
	prog2->addAttribute("aNor");
	prog2->addAttribute("aTex");
	prog2->addUniform("P");
	prog2->addUniform("MV");
	prog2->addUniform("T");
	prog2->addUniform("texture0");
	prog2->addUniform("texture1");
	prog2->addUniform("lightPosCam");
	prog2->setVerbose(false);

	glm::vec3 pos1(10.0, 10.0, 10.0);
	glm::vec3 col1(0.8, 0.8, 0.8);
	sun = Light(pos1, col1);

	camera = make_shared<Camera>();
	camera->setInitDistance(0.0f); // Camera's initial Z translation
	
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

	frust = make_shared<Shape>();
	frust->loadMesh(RESOURCE_DIR + "frust.obj");
	frust->init();

	ground = make_shared<Texture>();
	ground->setFilename(RESOURCE_DIR + "earthKd.jpg");
	ground->init();
	ground->setUnit(0);
	ground->setWrapModes(GL_REPEAT, GL_REPEAT);

	grounds = make_shared<Texture>();
	grounds->setFilename(RESOURCE_DIR + "earthKs.jpg");
	grounds->init();
	grounds->setUnit(1);
	grounds->setWrapModes(GL_REPEAT, GL_REPEAT);

	std::srand(std::time(nullptr));
	
	// Add each individual world object
	for(int i = 0; i < 10; i++) {
		for(int j = 0; j < 10; j++) {
			glm::vec3 rotation(0.0, 0.0, 0.0);
			glm::vec3 translation(i*2, 0.0, j*2);
			glm::vec3 scale(1.0, 1.0, 1.0);
			glm::vec3 ambient(0.2, 0.2, 0.2);
			glm::vec3 diffuse(((double) std::rand() / (RAND_MAX)), ((double) std::rand() / (RAND_MAX)), ((double) std::rand() / (RAND_MAX)));
			glm::vec3 specular(((double) std::rand() / (RAND_MAX)), ((double) std::rand() / (RAND_MAX)), ((double) std::rand() / (RAND_MAX)));
			double shininess = std::rand() % 500;
			if((i + j) % 2 == 0) {
				wobjs.emplace_back(rotation, translation, scale, shape, ambient, diffuse, specular, shininess);
			} else {
				wobjs.emplace_back(rotation, translation, scale, teapot, ambient, diffuse, specular, shininess);
			}
		}
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
		glm::vec3 translation(10.0, 0.0, 10.0);
		glm::vec3 scale(25.0, 1.0, 25.0);
		glm::vec3 ambient(0.0, 0.0, 0.0);
		glm::vec3 diffuse(0.0, 0.0, 0.0);
		glm::vec3 specular(0.0, 0.0, 0.0);
		double shininess = 1;
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
	double scale_mult = 1 + 0.10/2 + (0.10/2)*sin(2 * M_PI * 1 * t);
	glm::vec3 scale_multiple(scale_mult, scale_mult, scale_mult);

	glViewport(0, 0, width, height);
	
	// Matrix stacks
	auto P = make_shared<MatrixStack>();
	auto MV = make_shared<MatrixStack>();
	P->pushMatrix();
	camera->applyOrthoMatrix(P, 1.0);
		MV->pushMatrix();
			prog->bind();
			MV->translate(-0.7*((float)width/(float)height), 0.25, -2.0);
			MV->rotate(t, 0.0f, 1.0f, 0.0f);
			MV->scale(0.3, 0.3, 0.3);
			glUniformMatrix4fv(prog->getUniform("P"), 1, GL_FALSE, glm::value_ptr(P->topMatrix()));
			glUniformMatrix4fv(prog->getUniform("MV"), 1, GL_FALSE, glm::value_ptr(MV->topMatrix()));
			glUniformMatrix4fv(prog->getUniform("IT"), 1, GL_FALSE, glm::value_ptr(glm::inverse(glm::transpose(MV->topMatrix()))));
			glm::vec3 hud_light(0.0, 0.0, 0.0);
			glUniform3fv(prog->getUniform("lightPos"), 1, glm::value_ptr(hud_light));
			glUniform3fv(prog->getUniform("lcol"), 1, glm::value_ptr(sun.color));
			glUniform3fv(prog->getUniform("ka"), 1, glm::value_ptr(wobjs[0].ambient));
			glUniform3fv(prog->getUniform("kd"), 1, glm::value_ptr(wobjs[0].diffuse));
			glUniform3fv(prog->getUniform("ks"), 1, glm::value_ptr(wobjs[0].specular));
			glUniform1f(prog->getUniform("s"), wobjs[0].shiny);
			shape->draw(prog);
			prog->unbind();
		MV->popMatrix();
		
		MV->pushMatrix();
			prog->bind();
			MV->translate(0.7*((float)width/(float)height), 0.25, -2.0);
			MV->rotate(t, 0.0f, 1.0f, 0.0f);
			MV->scale(0.3, 0.3, 0.3);
			glUniformMatrix4fv(prog->getUniform("P"), 1, GL_FALSE, glm::value_ptr(P->topMatrix()));
			glUniformMatrix4fv(prog->getUniform("MV"), 1, GL_FALSE, glm::value_ptr(MV->topMatrix()));
			glUniformMatrix4fv(prog->getUniform("IT"), 1, GL_FALSE, glm::value_ptr(glm::inverse(glm::transpose(MV->topMatrix()))));
			glUniform3fv(prog->getUniform("lightPos"), 1, glm::value_ptr(hud_light));
			glUniform3fv(prog->getUniform("lcol"), 1, glm::value_ptr(sun.color));
			glUniform3fv(prog->getUniform("ka"), 1, glm::value_ptr(wobjs[0].ambient));
			glUniform3fv(prog->getUniform("kd"), 1, glm::value_ptr(wobjs[0].diffuse));
			glUniform3fv(prog->getUniform("ks"), 1, glm::value_ptr(wobjs[0].specular));
			glUniform1f(prog->getUniform("s"), wobjs[0].shiny);
			shape->draw(prog);
			prog->unbind();
		MV->popMatrix();
		
	P->popMatrix();
	
	// Apply camera transforms
	P->pushMatrix();
	camera->applyProjectionMatrix(P);
	MV->pushMatrix();
	camera->applyViewMatrix(MV);

	auto cam_matrix = glm::inverse(MV->topMatrix());

	// Handle the sun
	glm::vec4 sunpos_coord(sun.position.x, sun.position.y, sun.position.z, 1.0);
	glm::mat4 sun_matrix = MV->topMatrix();
	glm::vec3 sunpos = (sun_matrix * sunpos_coord);
	
	// Make the ground
	MV->pushMatrix();
		MV->translate(wobjs[wobjs.size()-1].translate);
		MV->scale(wobjs[wobjs.size()-1].scale);
		MV->rotate(3*(M_PI/2), 1.0, 0.0, 0.0);
		prog2->bind();
		glUniformMatrix4fv(prog2->getUniform("P"), 1, GL_FALSE, glm::value_ptr(P->topMatrix()));
		glUniformMatrix4fv(prog2->getUniform("MV"), 1, GL_FALSE, glm::value_ptr(MV->topMatrix()));
		glUniformMatrix4fv(prog2->getUniform("T"), 1, GL_FALSE, glm::value_ptr(glm::inverse(glm::transpose(MV->topMatrix()))));
		glUniform3fv(prog2->getUniform("lightPosCam"), 1, glm::value_ptr(sunpos));
		ground->bind(prog2->getUniform("texture0"));
		grounds->bind(prog2->getUniform("texture1"));
		wobjs[wobjs.size()-1].shape->draw(prog2);
		ground->unbind();
		grounds->unbind();
		prog2->unbind();
	MV->popMatrix();

	// Make the sun
	MV->pushMatrix();
		MV->translate(wobjs[wobjs.size()-2].translate);
		MV->scale(wobjs[wobjs.size()-2].scale);
		prog->bind();
		glUniformMatrix4fv(prog->getUniform("P"), 1, GL_FALSE, glm::value_ptr(P->topMatrix()));
		glUniformMatrix4fv(prog->getUniform("MV"), 1, GL_FALSE, glm::value_ptr(MV->topMatrix()));
		glUniformMatrix4fv(prog->getUniform("IT"), 1, GL_FALSE, glm::value_ptr(glm::inverse(glm::transpose(MV->topMatrix()))));
		glUniform3fv(prog->getUniform("lightPos"), 1, glm::value_ptr(sunpos));
		glUniform3fv(prog->getUniform("lcol"), 1, glm::value_ptr(sun.color));
		glUniform3fv(prog->getUniform("ka"), 1, glm::value_ptr(wobjs[wobjs.size()-2].ambient));
		glUniform3fv(prog->getUniform("kd"), 1, glm::value_ptr(wobjs[wobjs.size()-2].diffuse));
		glUniform3fv(prog->getUniform("ks"), 1, glm::value_ptr(wobjs[wobjs.size()-2].specular));
		glUniform1f(prog->getUniform("s"), wobjs[wobjs.size()-2].shiny);
		wobjs[wobjs.size()-2].shape->draw(prog);
		prog->unbind();
	MV->popMatrix();
	
	// Apply all transformations
	for(unsigned int i = 0; i < wobjs.size()-2; i++) {	
		MV->pushMatrix();
			MV->translate(wobjs[i].translate);
			MV->scale(wobjs[i].scale);
			MV->translate(0.0, (0.0-wobjs[i].shape->lowest_y)*scale_mult, 0.0);
			MV->scale(scale_multiple);
			prog->bind();
			glUniformMatrix4fv(prog->getUniform("P"), 1, GL_FALSE, glm::value_ptr(P->topMatrix()));
			glUniformMatrix4fv(prog->getUniform("MV"), 1, GL_FALSE, glm::value_ptr(MV->topMatrix()));
			glUniformMatrix4fv(prog->getUniform("IT"), 1, GL_FALSE, glm::value_ptr(glm::inverse(glm::transpose(MV->topMatrix()))));
			glUniform3fv(prog->getUniform("lightPos"), 1, glm::value_ptr(sunpos));
			glUniform3fv(prog->getUniform("lcol"), 1, glm::value_ptr(sun.color));
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


	if(keyToggles[(unsigned)'t']) {
		double s = 0.5;
		glViewport(0, 0, s*width, s*height);
		glEnable(GL_SCISSOR_TEST);
		glScissor(0, 0, s*width, s*height);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glDisable(GL_SCISSOR_TEST);
		P->pushMatrix();
		camera->applyOrthoMatrix(P, 15);
		MV->pushMatrix();
		camera->applyTopViewMatrix(MV);
			//frust
			MV->pushMatrix();
				MV->multMatrix(cam_matrix);
				MV->scale(camera->getAspect() * std::tan(camera->getFovy()/2), std::tan(camera->getFovy()/2), 1.0);
				prog->bind();
				glUniformMatrix4fv(prog->getUniform("P"), 1, GL_FALSE, glm::value_ptr(P->topMatrix()));
				glUniformMatrix4fv(prog->getUniform("MV"), 1, GL_FALSE, glm::value_ptr(MV->topMatrix()));
				glUniformMatrix4fv(prog->getUniform("IT"), 1, GL_FALSE, glm::value_ptr(glm::inverse(glm::transpose(MV->topMatrix()))));
				glm::vec3 blackest(0.0, 0.0, 0.0);
				glUniform3fv(prog->getUniform("ka"), 1, glm::value_ptr(blackest));
				glUniform3fv(prog->getUniform("kd"), 1, glm::value_ptr(blackest));
				glUniform3fv(prog->getUniform("ks"), 1, glm::value_ptr(blackest));
				glUniform3fv(prog->getUniform("lightPos"), 1, glm::value_ptr(sunpos));
				glUniform3fv(prog->getUniform("lcol"), 1, glm::value_ptr(sun.color));
				glUniform1f(prog->getUniform("s"), wobjs[wobjs.size()-1].shiny);
				frust->draw(prog);
				prog->unbind();
			MV->popMatrix();

			// Handle the sun
			glm::vec4 sunpos_coord(sun.position.x, sun.position.y, sun.position.z, 1.0);
			glm::mat4 sun_matrix = MV->topMatrix();
			glm::vec3 sunpos = (sun_matrix * sunpos_coord);
			
			// Make the ground
			MV->pushMatrix();
				MV->translate(wobjs[wobjs.size()-1].translate);
				MV->scale(wobjs[wobjs.size()-1].scale);
				MV->rotate(3*M_PI/2, 1.0, 0.0, 0.0);
				prog2->bind();
				glUniformMatrix4fv(prog2->getUniform("P"), 1, GL_FALSE, glm::value_ptr(P->topMatrix()));
				glUniformMatrix4fv(prog2->getUniform("MV"), 1, GL_FALSE, glm::value_ptr(MV->topMatrix()));
				glUniformMatrix4fv(prog2->getUniform("T"), 1, GL_FALSE, glm::value_ptr(glm::inverse(glm::transpose(MV->topMatrix()))));
				glUniform3fv(prog2->getUniform("lightPosCam"), 1, glm::value_ptr(sunpos));
				ground->bind(prog2->getUniform("texture0"));
				grounds->bind(prog2->getUniform("texture1"));
				wobjs[wobjs.size()-1].shape->draw(prog2);
				ground->unbind();
				grounds->unbind();
				prog2->unbind();
			MV->popMatrix();


			// Make the sun
			MV->pushMatrix();
				MV->translate(wobjs[wobjs.size()-2].translate);
				MV->scale(wobjs[wobjs.size()-2].scale);
				prog->bind();
				glUniformMatrix4fv(prog->getUniform("P"), 1, GL_FALSE, glm::value_ptr(P->topMatrix()));
				glUniformMatrix4fv(prog->getUniform("MV"), 1, GL_FALSE, glm::value_ptr(MV->topMatrix()));
				glUniformMatrix4fv(prog->getUniform("IT"), 1, GL_FALSE, glm::value_ptr(glm::inverse(glm::transpose(MV->topMatrix()))));
				glUniform3fv(prog->getUniform("lightPos"), 1, glm::value_ptr(sunpos));
				glUniform3fv(prog->getUniform("lcol"), 1, glm::value_ptr(sun.color));
				glUniform3fv(prog->getUniform("ka"), 1, glm::value_ptr(wobjs[wobjs.size()-2].ambient));
				glUniform3fv(prog->getUniform("kd"), 1, glm::value_ptr(wobjs[wobjs.size()-2].diffuse));
				glUniform3fv(prog->getUniform("ks"), 1, glm::value_ptr(wobjs[wobjs.size()-2].specular));
				glUniform1f(prog->getUniform("s"), wobjs[wobjs.size()-2].shiny);
				wobjs[wobjs.size()-2].shape->draw(prog);
				prog->unbind();
			MV->popMatrix();
			
			// Apply all transformations
			for(unsigned int i = 0; i < wobjs.size()-2; i++) {	
				MV->pushMatrix();
					MV->translate(wobjs[i].translate);
					MV->scale(wobjs[i].scale);
					MV->translate(0.0, (0.0-wobjs[i].shape->lowest_y)*scale_mult, 0.0);
					MV->scale(scale_multiple);
					prog->bind();
					glUniformMatrix4fv(prog->getUniform("P"), 1, GL_FALSE, glm::value_ptr(P->topMatrix()));
					glUniformMatrix4fv(prog->getUniform("MV"), 1, GL_FALSE, glm::value_ptr(MV->topMatrix()));
					glUniformMatrix4fv(prog->getUniform("IT"), 1, GL_FALSE, glm::value_ptr(glm::inverse(glm::transpose(MV->topMatrix()))));
					glUniform3fv(prog->getUniform("lightPos"), 1, glm::value_ptr(sunpos));
					glUniform3fv(prog->getUniform("lcol"), 1, glm::value_ptr(sun.color));
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
	}	

	
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
