#pragma once

#include "objects/Object3D.h"

/*!
 * Camera extends Object3D
 * Une camera est un objet3D disposant en plus d'une matrice de projection.
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
  void setEKey(bool);
  void setQKey(bool);
  void setPositionY(float y);

  void activeLook(bool);

  float getYaw();

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
};
