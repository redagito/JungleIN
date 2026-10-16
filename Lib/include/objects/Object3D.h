#pragma once

#include <glm/glm.hpp>

/*!
 * Object3D
 * Represents a 3D object holding a transformation matrix, and thus
 * position/rotation/scale vectors.
 * @abstract
 */
class Object3D {
public:
  Object3D();

  void setRotation(const glm::vec3 &);
  void setPosition(const glm::vec3 &);
  void setScale(const glm::vec3 &);
  // Logically const: only refreshes the mutable matrix cache.
  void updateMatrix() const;

  void rotate(const glm::vec3 &);
  void rotateX(float);
  void rotateY(float);
  void rotateZ(float);
  void translate(const glm::vec3 &);
  void translateX(float);
  void translateY(float);
  void translateZ(float);
  void translateRel(float, glm::vec3);
  void translateRelX(float);
  void translateRelY(float);
  void translateRelZ(float);

  glm::mat4 &getMatrix();
  const glm::mat4 &getMatrix() const;
  glm::vec3 &getPosition();
  const glm::vec3 &getPosition() const;

protected:
  // Mutable cache: getMatrix refreshes it even on const objects.
  mutable glm::mat4 matrix;
  glm::vec3 rotation;
  glm::vec3 position;
  glm::vec3 scale;

  mutable bool needUpdate = false;
  bool visible = false;
};
