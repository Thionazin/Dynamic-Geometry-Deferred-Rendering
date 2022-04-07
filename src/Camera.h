#pragma  once
#ifndef CAMERA_H
#define CAMERA_H

#include <memory>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>

class MatrixStack;

class Camera
{
public:
	enum {
		ROTATE = 0,
		TRANSLATE,
		SCALE
	};
	
	Camera();
	virtual ~Camera();
	void setInitDistance(float z) { translations.z = -std::abs(z); }
	void setAspect(float a) { aspect = a; };
	void setRotationFactor(float f) { rfactor = f; };
	void setTranslationFactor(float f) { tfactor = f; };
	void setScaleFactor(float f) { sfactor = f; };
	void mouseClicked(float x, float y, bool shift, bool ctrl, bool alt);
	void mouseMoved(float x, float y);
	void applyProjectionMatrix(std::shared_ptr<MatrixStack> P) const;
	void applyOrthoMatrix(std::shared_ptr<MatrixStack> P, float mult) const;
	void applyViewMatrix(std::shared_ptr<MatrixStack> MV) const;
	void applyTopViewMatrix(std::shared_ptr<MatrixStack> MV) const;
	void moveDir(float change);
	void moveSide(float change);
	void zoom(float change);
	float getFovy() { return fovy; };
	float getAspect() { return aspect; };
	
private:
	float aspect;
	float fovy;
	float znear;
	float zfar;
	glm::vec2 rotations;
	glm::vec3 translations;
	glm::vec2 mousePrev;
	int state;
	float rfactor;
	float tfactor;
	float sfactor;
	glm::vec3 world_pos;
	double yaw;
	double pitch;
};

#endif
