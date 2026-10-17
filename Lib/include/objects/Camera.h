#pragma once

#include <glm/glm.hpp>

#include "objects/Object3D.h"

/*!
 * Camera extends Object3D
 * A camera is an Object3D with an additional projection matrix.
 */
class Camera : public Object3D {
public:
  // Phase 4 unified projection defaults: single source of truth for the
  // constructor and Renderer::resize (was 90 vs 65). Far stays 500: the
  // skybox is a 1 m half-extent cube recentered on the camera every
  // frame, so 500 leaves ample margin without spending depth precision
  // on an 800 far plane.
  static constexpr float kDefaultFov = 65.0f;
  static constexpr float kDefaultNear = 0.1f;
  static constexpr float kDefaultFar = 500.0f;

  Camera();

  void setPerspective(float fov, float ratio, float near, float far);

  glm::mat4 &getProjection();
  const glm::mat4 &getProjection() const;

  void update(double groundY, double delta = 1.0);
  void reset();

  void setTarget(const glm::vec3 &_target);
  glm::vec3 getTarget() const;

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

  float getYaw() const;
  float getPitch() const;

private:
  glm::mat4 projection;

  glm::vec3 target;

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
};
