#include "objects/Light.h"
#include <cassert>
#include <limits>
#include <stdio.h>

#include <glm/gtc/matrix_transform.hpp>

namespace {
// Zero-length maps to the zero vector (no NaN); glm::normalize does not,
// so the guard is replicated here.
glm::vec3 safeNormalize(const glm::vec3 &v) {
  float len = glm::length(v);
  if (len > std::numeric_limits<float>::epsilon())
    return v / len;
  return glm::vec3(0.0f);
}
} // namespace

Light::Light(glm::vec3 position, glm::vec3 direction, glm::vec3 intensity) {
  this->position = position;
  this->direction = direction;
  this->intensity = intensity;

  size = 20;
  sizeCascade = 120;

  // Same depth ranges as resize(1) produces, so the first
  // Renderer::resize does not pop the shadow frustum.
  // Near/far derive from the Sun orbit (lightDistance 150 m, see
  // Sun.cpp, unchanged): inner 60..200 m brackets the 150 m throw
  // plus scene depth, cascade 40..300 m widens for the 120 m ortho
  // box. Ortho-only: w stays 1 through bias*ortho*view*model.
  // NOTE: glm::ortho takes (left, right, bottom, top).
  projMatrix = glm::ortho(-size, size, -size, size, 60.0f, 200.0f);
  projMatrixCascade =
      glm::ortho(-sizeCascade, sizeCascade, -sizeCascade, sizeCascade,
                 40.0f, 300.0f);

  // Row-major historical bias set(0.5,0,0,0.5, 0,0.5,0,0.5, 0,0,0.5,0.5, 0,0,0,1)
  // transposed to column-major glm columns.
  biasMatrix = glm::mat4(0.5f, 0.0f, 0.0f, 0.0f, //
                         0.0f, 0.5f, 0.0f, 0.0f, //
                         0.0f, 0.0f, 0.5f, 0.0f, //
                         0.5f, 0.5f, 0.5f, 1.0f);
  viewMatrix = glm::mat4(1.0f);
  textureProjMatrix = glm::mat4(1.0f);
  textureProjMatrixCascade = glm::mat4(1.0f);
}

void Light::setSize(float s) {
  size = s;
  // Rebuild immediately so a lone setSize never leaves a stale projection.
  resize(1.0f);
}

void Light::resize(float ratio) {
  projMatrix = glm::ortho(-size, size, -size / ratio, size / ratio, 60.0f,
                          200.0f);
  projMatrixCascade = glm::ortho(-sizeCascade, sizeCascade,
                                 -sizeCascade / ratio, sizeCascade / ratio,
                                 40.0f, 300.0f);
}

void Light::setDirection(const glm::vec3 &v) {
  this->direction = v;
}

glm::vec3 &Light::getDirection() { return direction; }

void Light::lookAt(const glm::vec3 &target) {
  direction = safeNormalize(target - position);

  // lookAt convention == glm::lookAt. The overhead fallback
  // (view direction parallel to up) is kept via the Z-up swap so an
  // exactly-overhead sun still yields a valid basis instead of NaNs.
  glm::vec3 up(0.0f, 1.0f, 0.0f);
  if (std::fabs(glm::dot(direction, up)) > 0.9999f)
    up = glm::vec3(0.0f, 0.0f, 1.0f);
  if (glm::length(target - position) <=
      std::numeric_limits<float>::epsilon())
    return;
  viewMatrix = glm::lookAt(position, target, up);
}

glm::vec3 &Light::getIntensity() { return intensity; }

void Light::setIntensity(const glm::vec3 &v) {
  this->intensity = v;
}

glm::vec3 &Light::getPosition() { return position; }

void Light::setPosition(const glm::vec3 &v) {
  this->position = v;
}

glm::mat4 &Light::getProjMatrix() { return projMatrix; }

glm::mat4 &Light::getProjMatrixCascade() { return projMatrixCascade; }

glm::mat4 &Light::getViewMatrix() { return viewMatrix; }

glm::mat4 &Light::getTextureProjMatrix(const glm::mat4 &modelMatrix) {
  textureProjMatrix = biasMatrix * projMatrix * viewMatrix * modelMatrix;
#ifndef NDEBUG
  // Ortho-only invariant: perspective divide must be a no-op (w == 1).
  // A perspective light would break this and needs divide-by-w here.
  // Column-major: the last row is m[0][3], m[1][3], m[2][3], m[3][3].
  assert(textureProjMatrix[0][3] == 0.0f);
  assert(textureProjMatrix[1][3] == 0.0f);
  assert(textureProjMatrix[2][3] == 0.0f);
  assert(textureProjMatrix[3][3] == 1.0f);
#endif
  return textureProjMatrix;
}

glm::mat4 &Light::getTextureProjMatrixCascade(const glm::mat4 &modelMatrix) {
  textureProjMatrixCascade =
      biasMatrix * projMatrixCascade * viewMatrix * modelMatrix;
#ifndef NDEBUG
  assert(textureProjMatrixCascade[0][3] == 0.0f);
  assert(textureProjMatrixCascade[1][3] == 0.0f);
  assert(textureProjMatrixCascade[2][3] == 0.0f);
  assert(textureProjMatrixCascade[3][3] == 1.0f);
#endif
  return textureProjMatrixCascade;
}
