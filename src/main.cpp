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
#include "Sphere.h"
#include "Revo.h"
#include "Texture.h"

#include "WorldObject.h"
#include "Light.h"

using namespace std;

GLFWwindow *window; // Main application window
string RESOURCE_DIR = "./"; // Where the resources are loaded from
bool OFFLINE = false;

shared_ptr<Camera> camera;
shared_ptr<Program> prog;
shared_ptr<Program> sp_prog;
shared_ptr<Program> prog_pass;

shared_ptr<Shape> shape;
shared_ptr<Shape> teapot;
shared_ptr<Shape> w_floor;
shared_ptr<Shape> sphere;
shared_ptr<Sphere> cust_sphere;
shared_ptr<Revo> spiral;

vector<glm::vec3> light_positions;
vector<glm::vec3> light_colors;

vector<WorldObject> wobjs;

int texWidth = 640;
int texHeight = 480;

GLuint framebufferID;
GLuint pos_tex;
GLuint nor_tex;
GLuint ke_tex;
GLuint kd_tex;

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
	texWidth = width;
	texHeight = height;

	//glGenFramebuffers(1, &framebufferID);
	glBindFramebuffer(GL_FRAMEBUFFER, framebufferID);

	// position texture
	glBindTexture(GL_TEXTURE_2D, pos_tex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, texWidth, texHeight, 0, GL_RGB, GL_FLOAT, NULL);

	// normal texture
	glBindTexture(GL_TEXTURE_2D, nor_tex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, texWidth, texHeight, 0, GL_RGB, GL_FLOAT, NULL);

	// ke texture
	glBindTexture(GL_TEXTURE_2D, ke_tex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, texWidth, texHeight, 0, GL_RGB, GL_FLOAT, NULL);

	// kd texture
	glBindTexture(GL_TEXTURE_2D, kd_tex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, texWidth, texHeight, 0, GL_RGB, GL_FLOAT, NULL);
	
	GLuint depthrenderbuffer;
	glGenRenderbuffers(1, &depthrenderbuffer);
	glBindRenderbuffer(GL_RENDERBUFFER, depthrenderbuffer);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, texWidth, texHeight);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthrenderbuffer);


	if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
	    cerr << "Framebuffer is not ok" << endl;
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

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
	prog->setShaderNames(RESOURCE_DIR + "bp_vert.glsl", RESOURCE_DIR + "dr_frag.glsl");
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

	sp_prog = make_shared<Program>();
	sp_prog->setShaderNames(RESOURCE_DIR + "vert.glsl", RESOURCE_DIR + "dr_frag.glsl");
	sp_prog->setVerbose(true);
	sp_prog->init();
	sp_prog->addAttribute("aPos");
	sp_prog->addAttribute("aNor");
	sp_prog->addAttribute("aTex");
	sp_prog->addUniform("MV");
	sp_prog->addUniform("P");
	sp_prog->addUniform("IT");
	sp_prog->addUniform("time");
	sp_prog->addUniform("light_positions");
	sp_prog->addUniform("light_colors");
	sp_prog->addUniform("ka");
	sp_prog->addUniform("kd");
	sp_prog->addUniform("ks");
	sp_prog->addUniform("s");
	sp_prog->setVerbose(false);

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
	std::uniform_real_distribution<> distrad(0.5, 1.0);

	cust_sphere = make_shared<Sphere>();
	cust_sphere->init(distrad(gen));

	spiral = make_shared<Revo>();
	spiral->init();
	
	// "random" colors aren't true random, I believe it's because it's using the same seed
	int counter = 0;
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
			if(counter % 4 == 0) {
				wobjs.emplace_back(rotation, translation, scale, shape, ambient, diffuse, specular, shininess);
			} else if(counter % 4 == 1) {
				wobjs.emplace_back(rotation, translation, scale, teapot, ambient, diffuse, specular, shininess);
			} else if(counter % 4 == 2) {
				glm::vec3 def_scale(0.5, 0.5, 0.5);
				wobjs.emplace_back(rotation, translation, scale*def_scale, cust_sphere, ambient, diffuse, specular, shininess);
			} else if(counter % 4 == 3) {
				glm::vec3 def_scale(0.15, 0.15, 0.15);
				wobjs.emplace_back(rotation, translation, scale*def_scale, spiral, ambient, diffuse, specular, shininess);
			}
			counter++;
		}
	}

	// Add the lights
	{
		light_positions.emplace_back(1.5, 0.3, 1.5);
		light_colors.emplace_back(1.0, 1.0, 1.0);
	}

	{
		light_positions.emplace_back(3.5, 0.3, 3.5);
		light_colors.emplace_back(0.2, 1.0, 0.2);
	}
	
	{
		light_positions.emplace_back(5.5, 0.3, 5.5);
		light_colors.emplace_back(0.2, 0.2, 1.0);
	}

	{
		light_positions.emplace_back(7.5, 0.3, 7.5);
		light_colors.emplace_back(0.8, 0.2, 1.0);
	}

	{
		light_positions.emplace_back(2.5, 0.3, 1.5);
		light_colors.emplace_back(0.8, 0.3, 0.3);
	}

	{
		light_positions.emplace_back(8.5, 0.3, 4.5);
		light_colors.emplace_back(0.5, 0.2, 0.6);
	}

	{
		light_positions.emplace_back(3.5, 0.3, 9.5);
		light_colors.emplace_back(0.5, 0.3, 0.8);
	}

	{
		light_positions.emplace_back(6.5, 0.3, 5.5);
		light_colors.emplace_back(0.1, 0.1, 0.5);
	}

	{
		light_positions.emplace_back(3.5, 0.3, 8.5);
		light_colors.emplace_back(0.9, 0.2, 0.8);
	}
	
	{
		light_positions.emplace_back(2.5, 0.3, 6.5);
		light_colors.emplace_back(0.2, 0.8, 0.8);
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

	glGenFramebuffers(1, &framebufferID);
	glBindFramebuffer(GL_FRAMEBUFFER, framebufferID);

	// position texture
	glGenTextures(1, &pos_tex);
	glBindTexture(GL_TEXTURE_2D, pos_tex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, texWidth, texHeight, 0, GL_RGB, GL_FLOAT, NULL);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pos_tex, 0);

	// normal texture
	glGenTextures(1, &nor_tex);
	glBindTexture(GL_TEXTURE_2D, nor_tex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, texWidth, texHeight, 0, GL_RGB, GL_FLOAT, NULL);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, nor_tex, 0);

	// ke texture
	glGenTextures(1, &ke_tex);
	glBindTexture(GL_TEXTURE_2D, ke_tex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, texWidth, texHeight, 0, GL_RGB, GL_FLOAT, NULL);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, ke_tex, 0);

	// kd texture
	glGenTextures(1, &kd_tex);
	glBindTexture(GL_TEXTURE_2D, kd_tex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, texWidth, texHeight, 0, GL_RGB, GL_FLOAT, NULL);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, GL_TEXTURE_2D, kd_tex, 0);
	
	GLuint depthrenderbuffer;
	glGenRenderbuffers(1, &depthrenderbuffer);
	glBindRenderbuffer(GL_RENDERBUFFER, depthrenderbuffer);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, texWidth, texHeight);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthrenderbuffer);

	GLenum attachments[] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3};
	glDrawBuffers(4, attachments);

	if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
	    cerr << "Framebuffer is not ok" << endl;
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);


	prog_pass = make_shared<Program>();
	prog_pass->setShaderNames(RESOURCE_DIR + "dr_vert.glsl", RESOURCE_DIR + "bp_frag.glsl");
	prog_pass->setVerbose(true);
	prog_pass->init();
	prog_pass->addAttribute("aPos");
	prog_pass->addUniform("MV");
	prog_pass->addUniform("P");
	prog_pass->addUniform("light_positions");
	prog_pass->addUniform("light_colors");
	prog_pass->addUniform("window_size");
	prog_pass->addUniform("ks");
	prog_pass->addUniform("s");
	prog_pass->setVerbose(false);
	prog_pass->addUniform("pos_tex");
	prog_pass->addUniform("nor_tex");
	prog_pass->addUniform("ke_tex");
	prog_pass->addUniform("kd_tex");
	prog_pass->bind();
	glUniform1i(prog_pass->getUniform("pos_tex"), 0);
	glUniform1i(prog_pass->getUniform("nor_tex"), 1);
	glUniform1i(prog_pass->getUniform("ke_tex"), 2);
	glUniform1i(prog_pass->getUniform("kd_tex"), 3);
	prog_pass->unbind();



	
	GLSL::checkError(GET_FILE_LINE);
}

// This function is called every frame to draw the scene.
static void render()
{

	double t = glfwGetTime();

	auto P = make_shared<MatrixStack>();
	auto MV = make_shared<MatrixStack>();

	int width, height;
	glfwGetFramebufferSize(window, &width, &height);
	camera->setAspect((float)width/(float)height);



	glBindFramebuffer(GL_FRAMEBUFFER, framebufferID);
	glViewport(0, 0, width, height);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glEnable(GL_DEPTH_TEST);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	// Matrix stacks
	
	// Apply camera transforms
	P->pushMatrix();
	camera->applyProjectionMatrix(P);
	MV->pushMatrix();
	camera->applyViewMatrix(MV);


	// Handle the lights
	glm::vec3 camera_lights[10];
	glm::mat4 light_matrix = MV->topMatrix();
	for(unsigned int i = 0; i < light_positions.size(); i++) {
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
		glUniform3fv(prog->getUniform("light_positions"), 10, glm::value_ptr(camera_lights[0]));
		glUniform3fv(prog->getUniform("light_colors"), 10, glm::value_ptr(light_colors.data()[0]));
		glUniform3fv(prog->getUniform("ka"), 1, glm::value_ptr(wobjs[wobjs.size()-1].ambient));
		glUniform3fv(prog->getUniform("kd"), 1, glm::value_ptr(wobjs[wobjs.size()-1].diffuse));
		glUniform3fv(prog->getUniform("ks"), 1, glm::value_ptr(wobjs[wobjs.size()-1].specular));
		glUniform1f(prog->getUniform("s"), wobjs[wobjs.size()-1].shiny);
		wobjs[wobjs.size()-1].shape->draw(prog);
		prog->unbind();
	MV->popMatrix();

	// Make the lights
	for(unsigned int i = 0; i < light_positions.size(); i++) {
		MV->pushMatrix();
			MV->translate(light_positions[i]);
			MV->scale(0.1, 0.1, 0.1);
			prog->bind();
			glUniformMatrix4fv(prog->getUniform("P"), 1, GL_FALSE, glm::value_ptr(P->topMatrix()));
			glUniformMatrix4fv(prog->getUniform("MV"), 1, GL_FALSE, glm::value_ptr(MV->topMatrix()));
			glUniformMatrix4fv(prog->getUniform("IT"), 1, GL_FALSE, glm::value_ptr(glm::inverse(glm::transpose(MV->topMatrix()))));
			glUniform3fv(prog->getUniform("light_positions"), 10, glm::value_ptr(camera_lights[0]));
			glUniform3fv(prog->getUniform("light_colors"), 10, glm::value_ptr(light_colors.data()[0]));
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
	for(unsigned int i = 0; i < wobjs.size()-1; i++) {	
		MV->pushMatrix();
			MV->translate(wobjs[i].translate);
			if(wobjs[i].shape_type == 0) {
				MV->translate(0.0, (0.0-wobjs[i].shape->lowest_y)*wobjs[i].scale.y, 0.0);
				if(wobjs[i].shape == shape) {
					MV->rotate(t, 0.0, 1.0, 0.0);
				} else if(wobjs[i].shape == teapot) {
					glm::mat4 S(1.0f);
					S[1][2] = 0.5f*cos(t);
					MV->multMatrix(S);
				}
			} else if(wobjs[i].shape_type == 1) {
				MV->translate(0.0, (0.0-wobjs[i].c_sphere->lowest_y)*wobjs[i].scale.y, 0.0);
				MV->translate(0.0, 0.4*(0.5 * sin((2.0*M_PI)/(1.7)*(t+0.9)) + 0.5), 0.0);
				double scale_val = -0.5*(0.5*cos((4.0*M_PI)/(1.7)*(t+0.9))+0.5)+1.0;
				MV->scale(scale_val, 1.0, scale_val);
			} else if(wobjs[i].shape_type == 2) {
				MV->rotate(0.5 * M_PI, 0.0, 0.0, 1.0);
			}
			MV->scale(wobjs[i].scale);
			prog->bind();
			glUniformMatrix4fv(prog->getUniform("P"), 1, GL_FALSE, glm::value_ptr(P->topMatrix()));
			glUniformMatrix4fv(prog->getUniform("MV"), 1, GL_FALSE, glm::value_ptr(MV->topMatrix()));
			glUniformMatrix4fv(prog->getUniform("IT"), 1, GL_FALSE, glm::value_ptr(glm::inverse(glm::transpose(MV->topMatrix()))));
			glUniform3fv(prog->getUniform("light_positions"), 10, glm::value_ptr(camera_lights[0]));
			glUniform3fv(prog->getUniform("light_colors"), 10, glm::value_ptr(light_colors.data()[0]));
			glUniform3fv(prog->getUniform("ka"), 1, glm::value_ptr(wobjs[i].ambient));
			glUniform3fv(prog->getUniform("kd"), 1, glm::value_ptr(wobjs[i].diffuse));
			glUniform3fv(prog->getUniform("ks"), 1, glm::value_ptr(wobjs[i].specular));
			glUniform1f(prog->getUniform("s"), wobjs[i].shiny);
			if(wobjs[i].shape_type == 0) {
				wobjs[i].shape->draw(prog);
			} else if(wobjs[i].shape_type == 1) {
				cust_sphere->draw(prog);
			}
			prog->unbind();
			if(wobjs[i].shape_type == 2) {
				sp_prog->bind();
				glUniformMatrix4fv(sp_prog->getUniform("P"), 1, GL_FALSE, glm::value_ptr(P->topMatrix()));
				glUniformMatrix4fv(sp_prog->getUniform("MV"), 1, GL_FALSE, glm::value_ptr(MV->topMatrix()));
				glUniformMatrix4fv(sp_prog->getUniform("IT"), 1, GL_FALSE, glm::value_ptr(glm::inverse(glm::transpose(MV->topMatrix()))));
				glUniform1f(sp_prog->getUniform("time"), t);
				glUniform3fv(sp_prog->getUniform("light_positions"), 10, glm::value_ptr(camera_lights[0]));
				glUniform3fv(sp_prog->getUniform("light_colors"), 10, glm::value_ptr(light_colors.data()[0]));
				glUniform3fv(sp_prog->getUniform("ka"), 1, glm::value_ptr(wobjs[i].ambient));
				glUniform3fv(sp_prog->getUniform("kd"), 1, glm::value_ptr(wobjs[i].diffuse));
				glUniform3fv(sp_prog->getUniform("ks"), 1, glm::value_ptr(wobjs[i].specular));
				glUniform1f(sp_prog->getUniform("s"), wobjs[i].shiny);
				spiral->draw(sp_prog);
				sp_prog->unbind();
			}
		MV->popMatrix();
	}

	MV->popMatrix();
	P->popMatrix();

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, width, height);
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	glEnable(GL_DEPTH_TEST);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	MV->pushMatrix();
		prog_pass->bind();
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, pos_tex);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, nor_tex);
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, ke_tex);
		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_2D, kd_tex);
		glm::vec2 wind_size(texWidth, texHeight);
		MV->scale(2.0, 2.0, 2.0);
		glUniformMatrix4fv(prog_pass->getUniform("P"), 1, GL_FALSE, glm::value_ptr(P->topMatrix()));
		glUniformMatrix4fv(prog_pass->getUniform("MV"), 1, GL_FALSE, glm::value_ptr(MV->topMatrix()));
		glUniform3fv(prog_pass->getUniform("light_positions"), 10, glm::value_ptr(camera_lights[0]));
		glUniform3fv(prog_pass->getUniform("light_colors"), 10, glm::value_ptr(light_colors.data()[0]));
		glUniform2fv(prog_pass->getUniform("window_size"), 1, glm::value_ptr(wind_size));
		glUniform3fv(prog_pass->getUniform("ks"), 1, glm::value_ptr(wobjs[0].specular));
		glUniform1f(prog_pass->getUniform("s"), wobjs[0].shiny);
		w_floor->draw(prog_pass);
		glActiveTexture(GL_TEXTURE0);
		prog_pass->unbind();
	MV->popMatrix();



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
		cout << "Usage: A5 RESOURCE_DIR" << endl;
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
