#include "objects/Object3D.h"

#include <cmath>
#include <limits>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

namespace {
// Euler-to-quaternion with degrees input and the historical half-angle
// products, then mat4_cast.
glm::quat EulerToQuat(const glm::vec3 &v) {
  constexpr float kDegToHalfRad =
      3.14159265358979323846f / 360.0f;
  const float ex = v.x * kDegToHalfRad;
  const float ey = v.y * kDegToHalfRad;
  const float ez = v.z * kDegToHalfRad;
  const float c1 = std::cos(ey);
  const float s1 = std::sin(ey);
  const float c2 = std::cos(-ez);
  const float s2 = std::sin(-ez);
  const float c3 = std::cos(ex);
  const float s3 = std::sin(ex);
  const float c1c2 = c1 * c2;
  const float s1s2 = s1 * s2;
  return glm::quat(c1c2 * c3 - s1s2 * s3, c1c2 * s3 + s1s2 * c3,
                   s1 * c2 * c3 + c1 * s2 * s3,
                   c1 * s2 * c3 - s1 * c2 * s3);
}
} // namespace

Object3D::Object3D() {
  matrix = glm::mat4(1.0f);
  rotation = glm::vec3(0.0f);
  scale = glm::vec3(1.0f);
  position = glm::vec3(0.0f);
  needUpdate = false;
  visible = true;
}

void Object3D::setRotation(const glm::vec3 &v) {
  this->rotation = v;
  this->needUpdate = true;
}

void Object3D::setPosition(const glm::vec3 &v) {
  this->position = v;
  this->needUpdate = true;
}

void Object3D::setScale(const glm::vec3 &v) {
  this->scale = v;
  this->needUpdate = true;
}

void Object3D::updateMatrix() {
  // Model is T * R * S.
  this->matrix = glm::translate(glm::mat4(1.0f), this->position) *
                 glm::mat4_cast(EulerToQuat(this->rotation)) *
                 glm::scale(glm::mat4(1.0f), this->scale);

  this->needUpdate = false;
}

void Object3D::rotate(const glm::vec3 &xyz) {
  this->rotation += xyz;
  this->needUpdate = true;
}

void Object3D::translate(const glm::vec3 &xyz) {
  this->position += xyz;
  this->needUpdate = true;
}

void Object3D::rotateX(float s) {
  this->rotation.x += s;
  this->needUpdate = true;
}

void Object3D::rotateY(float s) {
  this->rotation.y += s;
  this->needUpdate = true;
}

void Object3D::rotateZ(float s) {
  this->rotation.z += s;
  this->needUpdate = true;
}

void Object3D::translateX(float s) {
  this->position.x += s;
  this->needUpdate = true;
}

void Object3D::translateY(float s) {
  this->position.y += s;
  this->needUpdate = true;
}

void Object3D::translateZ(float s) {
  this->position.z += s;
  this->needUpdate = true;
}

void Object3D::translateRel(float s, glm::vec3 axis) {
  // Rows of a column-major glm matrix are the transposed columns below.
  glm::vec3 row0(matrix[0][0], matrix[1][0], matrix[2][0]);
  glm::vec3 row1(matrix[0][1], matrix[1][1], matrix[2][1]);
  glm::vec3 row2(matrix[0][2], matrix[1][2], matrix[2][2]);
  axis = glm::vec3(glm::dot(row0, axis), glm::dot(row1, axis),
                   glm::dot(row2, axis));
  float len = glm::length(axis);
  if (len > std::numeric_limits<float>::epsilon())
    axis /= len;
  else
    axis = glm::vec3(0.0f);
  position += axis * s;
  this->needUpdate = true;
}

void Object3D::translateRelX(float s) { translateRel(s, glm::vec3(1, 0, 0)); }

void Object3D::translateRelY(float s) { translateRel(s, glm::vec3(0, 1, 0)); }

void Object3D::translateRelZ(float s) { translateRel(s, glm::vec3(0, 0, 1)); }

glm::mat4 &Object3D::getMatrix() {
  if (needUpdate)
    updateMatrix();
  return this->matrix;
}

glm::vec3 &Object3D::getPosition() { return position; }
