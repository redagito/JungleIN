#include "objects/Camera.h"
#include "helpers/Logger.h"
#include <cmath>

#define DEG_TO_RAD 3.141592653 / 180.0

Camera::Camera() : Object3D() {
	leftKey = false;
	rightKey = false;
	upKey = false;
	downKey = false;

	setPerspective(kDefaultFov, 1.33f, kDefaultNear, kDefaultFar);
	target.set(0, 0, 0);

	lookSpeed = 0.5f;
	moveSpeed = 200.f;

	look = false;

	height = 2.2f;
}

void Camera::reset() {
	yaw = 0.0;
	pitch = 0.0;
	heightOffset = 0.0;
	position.set(0, 6, -1);
	logi("Reset Camera");
}

void Camera::setPerspective(float fov, float ratio, float near, float far) {
	projection.setPerspective(fov, ratio, near, far);
}

Matrix4& Camera::getProjection() { return projection; }

void Camera::setLeftKey(bool a) { leftKey = a; }

void Camera::setRightKey(bool a) { rightKey = a; }

void Camera::setUpKey(bool a) { upKey = a; }

void Camera::setDownKey(bool a) { downKey = a; }

void Camera::setSprintKey(bool a) { sprintKey = a; }

void Camera::setEKey(bool b)
{
	eKey = b;
}

void Camera::setQKey(bool b)
{
	qKey = b;
}

void Camera::update(double groundY, double delta) {
	// http://www.lloydgoodall.com/tutorials/first-person-camera-control-with-lwjgl/

	// Min position
	//

	if (look) {
		yaw += accelX * lookSpeed;
		pitch += accelY * lookSpeed;
		// Never roll over past vertical: beyond +-89 degrees the view
		// flips behind and the forward-based culling breaks.
		if (pitch > 89.f) pitch = 89.f;
		if (pitch < -89.f) pitch = -89.f;
		// Deltas are per mouse event: consume them so the view does not
		// keep turning after the mouse stops.
		accelX = 0.f;
		accelY = 0.f;
	}
	else {
		// Motion while not looking is forgotten, otherwise re-enabling
		// look would apply it all at once as a view kick.
		accelX = 0.f;
		accelY = 0.f;
	}

	// Sprint (Shift held) doubles horizontal movement speed.
	const float speed = sprintKey ? moveSpeed * 2.0f : moveSpeed;

	if (leftKey) {
		position.x -= static_cast<float>(speed * std::sin((yaw + 90) * DEG_TO_RAD) * delta);
		position.z += static_cast<float>(speed * std::cos((yaw + 90) * DEG_TO_RAD) * delta);
	}

	if (rightKey) {
		position.x -= static_cast<float>(speed * std::sin((yaw - 90) * DEG_TO_RAD) * delta);
		position.z += static_cast<float>(speed * std::cos((yaw - 90) * DEG_TO_RAD) * delta);
	}

	if (upKey) {
		position.x += static_cast<float>(speed * std::sin(yaw * DEG_TO_RAD) * delta);
		position.z -= static_cast<float>(speed * std::cos(yaw * DEG_TO_RAD) * delta);
	}

	if (downKey) {
		position.x -= static_cast<float>(speed * std::sin(yaw * DEG_TO_RAD) * delta);
		position.z += static_cast<float>(speed * std::cos(yaw * DEG_TO_RAD) * delta);
	}

	// Fly higher / lower: adjusts the offset above the terrain.
	if (eKey)
	{
		heightOffset += static_cast<float>(moveSpeed * delta);
		if (heightOffset > 200.f) heightOffset = 200.f;
	}

	if (qKey)
	{
		heightOffset -= static_cast<float>(moveSpeed * delta);
		if (heightOffset < 0.f) heightOffset = 0.f;
	}

	// Walking stays glued to the terrain, uphill and downhill.
	position.y = static_cast<float>(groundY + height + heightOffset);

	matrix.identity()
		.translate(position * -1)
		.rotate(yaw, Vector3::Y)
		.rotate(pitch, Vector3::X);
}

void Camera::setTarget(const Vector3& _target) {
	target.set(_target.x, _target.y, _target.z);
}

Vector3 Camera::getTarget() { return target; }

void Camera::setMouse(int x, int y) {
	mouseXold = mouseX;
	mouseYold = mouseY;
	mouseX = (float)x;
	mouseY = (float)y;
	accelX = mouseX - mouseXold;
	accelY = mouseY - mouseYold;
}

void Camera::setLookAccel(int x, int y) {
	accelX = (float)x;
	accelY = (float)y;
}

void Camera::activeLook(bool active) { look = active; }

float Camera::getYaw() { return static_cast<float>(yaw * DEG_TO_RAD); }

float Camera::getPitch() { return static_cast<float>(pitch * DEG_TO_RAD); }

void Camera::setPositionY(float y) { this->position.y = y; }
