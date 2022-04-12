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
		std::shared_ptr<Sphere> c_sphere;
		std::shared_ptr<Revo> revo;
		glm::vec3 ambient;
		glm::vec3 diffuse;
		glm::vec3 specular;
		double shiny;
		int shape_type;
		WorldObject(glm::vec3 rot, glm::vec3 trans, glm::vec3 scal, std::shared_ptr<Shape> sha, glm::vec3 am, glm::vec3 diff, glm::vec3 spec, double s) : rotate(rot), translate(trans), scale(scal), shape(sha), ambient(am), diffuse(diff), specular(spec), shiny(s), shape_type(0) {};
		WorldObject(glm::vec3 rot, glm::vec3 trans, glm::vec3 scal, std::shared_ptr<Sphere> sha, glm::vec3 am, glm::vec3 diff, glm::vec3 spec, double s) : rotate(rot), translate(trans), scale(scal), c_sphere(sha), ambient(am), diffuse(diff), specular(spec), shiny(s), shape_type(1) {};
		WorldObject(glm::vec3 rot, glm::vec3 trans, glm::vec3 scal, std::shared_ptr<Revo> rev, glm::vec3 am, glm::vec3 diff, glm::vec3 spec, double s) : rotate(rot), translate(trans), scale(scal), revo(rev), ambient(am), diffuse(diff), specular(spec), shiny(s), shape_type(2) {};
};
