#pragma once

#include "objects/Object3D.h"

/*!
 * Camera extends Object3D
 * A camera is an Object3D with an additional projection matrix.
 */
class Camera : public Object3D {
public:
  Camera();

  void setPerspective(float fov, float ratio, float near, float far);

  Matrix4 &getProjection();

  void update(double groundY, double delta = 1.0);
  void reset();

  void setTarget(const Vector3 &_target);
  Vector3 getTarget();

  void setMouse(int x, int y);
  void setLookAccel(int x, int y);

  void setLeftKey(bool);
  void setRightKey(bool);
  void setUpKey(bool);
  void setDownKey(bool);
  void setSprintKey(bool);
  void setEKey(bool);
  void setQKey(bool);
  void setPositionY(float y);

  void activeLook(bool);

  float getYaw();
  float getPitch();

private:
  Matrix4 projection;

  Vector3 target;

  // Camera look around control
  bool look = false;

  // Movement
  bool leftKey = false;
  bool rightKey = false;
  bool upKey = false;
  bool downKey = false;
  bool sprintKey = false;
  // Fly up / down
  bool eKey = false;
  bool qKey = false;

  float mouseXold = 0.f;
  float mouseYold = 0.f;
  float mouseX = 0.f;
  float mouseY = 0.f;
  float accelX = 0.f;
  float accelY = 0.f;

  float lookSpeed = 0.f;
  float moveSpeed = 0.f;
  float yaw = 0.f;
  float pitch = 0.f;
  float height = 0.f;
  // Extra eye height above the terrain, adjusted with E/Q. Walking
  // always tracks groundY + height + heightOffset, so the camera
  // sticks to the terrain downhill too instead of floating.
  float heightOffset = 0.f;
};
