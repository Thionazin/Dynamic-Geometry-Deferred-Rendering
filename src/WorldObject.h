#pragma once

#include <glm/glm.hpp>
#include "Shape.h"
#include <memory>

class WorldObject
{
	public:
		glm::vec3 rotate;
		glm::vec3 translate;
		glm::vec3 scale;
		std::shared_ptr<Shape> shape;
		glm::vec3 ambient;
		glm::vec3 diffuse;
		glm::vec3 specular;
		double shiny;
		WorldObject(glm::vec3 rot, glm::vec3 trans, glm::vec3 scal, std::shared_ptr<Shape> sha, glm::vec3 am, glm::vec3 diff, glm::vec3 spec, double s) : rotate(rot), translate(trans), scale(scal), shape(sha), ambient(am), diffuse(diff), specular(spec), shiny(s) {};
};
