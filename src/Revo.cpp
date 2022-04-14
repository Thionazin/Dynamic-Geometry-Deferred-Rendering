#include "Revo.h"

#include <cmath>
#include "GLSL.h"
#include <glm/glm.hpp>

using namespace std;

Revo::Revo() : posBufID(0), norBufID(0), texBufID(0), indBufID(0) {}

Revo::~Revo() {}

void Revo::init() {

	vector<vector<unsigned int>> indStore;

	
	int intervals = 40;

	int counter = 0;
	for(double i = 0; i < 10; i+= 0.2) {
		vector<unsigned int> r_store;
		for(int j = 0; j < intervals; j++) {
			double x = i;
			double theta = (((double)j)/((double)intervals-1)) * 2 * M_PI;
			posBuf.push_back(x);
			posBuf.push_back(theta);
			posBuf.push_back(0.0f);
			norBuf.push_back(0.0f);
			norBuf.push_back(0.0f);
			norBuf.push_back(0.0f);
			texBuf.push_back(theta);
			texBuf.push_back(x);
			r_store.push_back(counter++);
		}
		indStore.push_back(r_store);
	}

	for(int i = 0; i < (10.0/0.2)-1.0; i++) {
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

void Revo::draw(const std::shared_ptr<Program> prog) const {
	// Bind position buffer
	int h_pos = prog->getAttribute("aPos");
	GLSL::checkError(GET_FILE_LINE);
	glEnableVertexAttribArray(h_pos);
	GLSL::checkError(GET_FILE_LINE);
	glBindBuffer(GL_ARRAY_BUFFER, posBufID);
	GLSL::checkError(GET_FILE_LINE);
	glVertexAttribPointer(h_pos, 3, GL_FLOAT, GL_FALSE, 0, (const void *)0);

	GLSL::checkError(GET_FILE_LINE);
	
	GLSL::checkError(GET_FILE_LINE);
	
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indBufID);

	int indCount = (int)indBuf.size();
	// Draw
	// int count = posBuf.size()/3; // number of indices to be rendered
	glDrawElements(GL_TRIANGLES, indCount, GL_UNSIGNED_INT, (const void* )0);

	GLSL::checkError(GET_FILE_LINE);
	
	// Disable and unbind
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glDisableVertexAttribArray(h_pos);
	
	GLSL::checkError(GET_FILE_LINE);
}
