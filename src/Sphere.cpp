#include "Sphere.h"

#include <cmath>
#include "GLSL.h"
#include <glm/glm.hpp>

using namespace std;

Sphere::Sphere() : posBufID(0), norBufID(0), texBufID(0), indBufID(0) {}

Sphere::~Sphere() {}

void Sphere::init(double radius) {

	lowest_y = -radius;

	vector<vector<unsigned int>> indStore;


	int intervals = 50;


	int counter = 0;
	for(int i = intervals-1; i >= 0; i--) {
		vector<unsigned int> r_store;
		for(int j = 0; j < intervals; j++) {
			double theta = (((double)i)/((double)intervals-1)) * M_PI;
			double phi = (((double)j)/((double)intervals-1)) * M_PI * 2.0;
			double x = radius * sin(theta) * sin(phi);
			double y = radius * cos(theta);
			double z = radius * sin(theta) * cos(phi);
			posBuf.push_back(x);
			posBuf.push_back(y);
			posBuf.push_back(z);
			norBuf.push_back(x);
			norBuf.push_back(y);
			norBuf.push_back(z);
			texBuf.push_back((((double)j)/((double)intervals-1)));
			texBuf.push_back((((double)(intervals-1-i))/((double)intervals-1)));
			r_store.push_back(counter++);
		}
		indStore.push_back(r_store);
	}

	for(int i = 0; i < intervals-1; i++) {
		for(int j = 0; j < intervals-1; j++) {
			indBuf.push_back(indStore[i][j]);
			indBuf.push_back(indStore[i][j+1]);
			indBuf.push_back(indStore[i+1][j+1]);
			indBuf.push_back(indStore[i][j]);
			indBuf.push_back(indStore[i+1][j+1]);
			indBuf.push_back(indStore[i+1][j]);
		}
	}

	// Send the position array to the GPU
	glGenBuffers(1, &posBufID);
	glBindBuffer(GL_ARRAY_BUFFER, posBufID);
	glBufferData(GL_ARRAY_BUFFER, posBuf.size()*sizeof(float), &posBuf[0], GL_STATIC_DRAW);
	
	// Send the normal array to the GPU
	glGenBuffers(1, &norBufID);
	glBindBuffer(GL_ARRAY_BUFFER, norBufID);
	glBufferData(GL_ARRAY_BUFFER, norBuf.size()*sizeof(float), &norBuf[0], GL_STATIC_DRAW);
	
	// Send the texture array to the GPU
	glGenBuffers(1, &texBufID);
	glBindBuffer(GL_ARRAY_BUFFER, texBufID);
	glBufferData(GL_ARRAY_BUFFER, texBuf.size()*sizeof(float), &texBuf[0], GL_STATIC_DRAW);

	glGenBuffers(1, &indBufID);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indBufID);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indBuf.size()*sizeof(unsigned int), &indBuf[0], GL_STATIC_DRAW);
	
	// Unbind the arrays
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	GLSL::checkError(GET_FILE_LINE);
}

void Sphere::draw(const std::shared_ptr<Program> prog) const {
	// Bind position buffer
	int h_pos = prog->getAttribute("aPos");
	glEnableVertexAttribArray(h_pos);
	glBindBuffer(GL_ARRAY_BUFFER, posBufID);
	glVertexAttribPointer(h_pos, 3, GL_FLOAT, GL_FALSE, 0, (const void *)0);
	
	// Bind normal buffer
	int h_nor = prog->getAttribute("aNor");
	glEnableVertexAttribArray(h_nor);
	glBindBuffer(GL_ARRAY_BUFFER, norBufID);
	glVertexAttribPointer(h_nor, 3, GL_FLOAT, GL_FALSE, 0, (const void *)0);

	GLSL::checkError(GET_FILE_LINE);
	
	// Bind texcoords buffer
	// Textures not needed as of now
	/*
	int h_tex = prog->getAttribute("aTex");
	glEnableVertexAttribArray(h_tex);
	glBindBuffer(GL_ARRAY_BUFFER, texBufID);
	glVertexAttribPointer(h_tex, 2, GL_FLOAT, GL_FALSE, 0, (const void *)0);
	*/

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indBufID);

	int indCount = (int)indBuf.size();
	// Draw
	// int count = posBuf.size()/3; // number of indices to be rendered
	glDrawElements(GL_TRIANGLES, indCount, GL_UNSIGNED_INT, (const void* )0);

	GLSL::checkError(GET_FILE_LINE);
	
	// Disable and unbind
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	//glDisableVertexAttribArray(h_tex);
	glDisableVertexAttribArray(h_nor);
	glDisableVertexAttribArray(h_pos);
	
	GLSL::checkError(GET_FILE_LINE);
}
