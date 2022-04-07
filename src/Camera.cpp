#define _USE_MATH_DEFINES
#include <cmath> 
#include <iostream>
#include <glm/gtc/matrix_transform.hpp>
#include "Camera.h"
#include "MatrixStack.h"

Camera::Camera() :
	aspect(1.0f),
	fovy((float)(45.0*M_PI/180.0)),
	znear(0.1f),
	zfar(1000.0f),
	rotations(0.0, 0.0),
	translations(0.0f, 0.0f, -5.0f),
	rfactor(0.01f),
	tfactor(0.001f),
	sfactor(0.005f),
	world_pos(glm::vec3(1.0, 2.0, 1.0)),
	yaw(0.0),
	pitch(0.0)
{
}

Camera::~Camera()
{
}

void Camera::mouseClicked(float x, float y, bool shift, bool ctrl, bool alt)
{
	mousePrev.x = x;
	mousePrev.y = y;
	if(shift) {
		state = Camera::TRANSLATE;
	} else if(ctrl) {
		state = Camera::SCALE;
	} else {
		state = Camera::ROTATE;
	}
}

void Camera::mouseMoved(float x, float y)
{
	/*
	switch(state) {
		case Camera::ROTATE:
			rotations += rfactor * dv;
			break;
		case Camera::TRANSLATE:
			translations.x -= translations.z * tfactor * dv.x;
			translations.y += translations.z * tfactor * dv.y;
			break;
		case Camera::SCALE:
			translations.z *= (1.0f - sfactor * dv.y);
			break;
	}
	*/
	glm::vec2 mouseCurr(x, y);
	glm::vec2 dv = mouseCurr - mousePrev;
	yaw -= 0.01 * dv.x;
	pitch -= 0.01 * dv.y;
	if(pitch > 1) {
		pitch = 1;
	} else if(pitch < -1) {
		pitch = -1;
	}
	mousePrev = mouseCurr;
}

void Camera::moveDir(float change)
{
	glm::vec3 forward(std::sin(yaw), 0, std::cos(yaw));
	world_pos += change * forward; 
}

void Camera::moveSide(float change)
{
	glm::vec3 forward(std::sin(yaw), 0, std::cos(yaw));
	glm::vec3 crossed = cross(forward, glm::vec3(0, 1, 0));
	world_pos += change * crossed;
}

void Camera::zoom(float change) {
	float fovy_deg = (fovy * 180) / M_PI;
	fovy_deg += change;
	if(fovy_deg > 114) {
		fovy_deg = 114;
	} else if(fovy_deg < 4) {
		fovy_deg = 4;
	}
	fovy = (fovy_deg * M_PI) / 180;
}

void Camera::applyProjectionMatrix(std::shared_ptr<MatrixStack> P) const
{
	// Modify provided MatrixStack
	P->multMatrix(glm::perspective(fovy, aspect, znear, zfar));
}

void Camera::applyOrthoMatrix(std::shared_ptr<MatrixStack> P, float mult) const {
	P->multMatrix(glm::ortho(-aspect*mult, aspect*mult, -mult, mult, znear, zfar));
}

void Camera::applyViewMatrix(std::shared_ptr<MatrixStack> MV) const
{
	//MV->translate(translations);
	glm::vec3 forward(std::sin(yaw), std::sin(pitch), std::cos(yaw));
	MV->multMatrix(glm::lookAt(world_pos, world_pos+forward, glm::vec3(0.0, 1.0, 0.0)));
	//MV->rotate(rotations.y, glm::vec3(1.0f, 0.0f, 0.0f));
	//MV->rotate(rotations.x, glm::vec3(0.0f, 1.0f, 0.0f));
}

void Camera::applyTopViewMatrix(std::shared_ptr<MatrixStack> MV) const
{
	MV->translate(0.0, 0.0, -200.0);
	MV->rotate(1*(0.5*M_PI), 1.0, 0.0, 0.0);
	MV->translate(-10.0, -1.0, -10.0);
}
