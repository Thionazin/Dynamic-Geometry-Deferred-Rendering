#pragma once

#include <glm/glm.hpp>

class Light 
{
	public:
		glm::vec3 position;
		glm::vec3 color;
		Light() : position(0.0), color(0.0) {};
		Light(glm::vec3 pos, glm::vec3 col) : position(pos), color(col) {};
};
