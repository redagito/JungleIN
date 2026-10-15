#include "objects/Light.h"
#include <cassert>
#include <stdio.h>

Light::Light(Vector3 position, Vector3 direction, Vector3 intensity) {
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
  projMatrix.setOrthogonal(-size, size, size, -size, 60, 200);
  projMatrixCascade.setOrthogonal(-sizeCascade, sizeCascade, sizeCascade,
                                  -sizeCascade, 40, 300);

  biasMatrix.set(0.5f, 0.0f, 0.0f, 0.5f, 0.0f, 0.5f, 0.0f, 0.5f, 0.0f, 0.0f, 0.5f, 0.5f,
                 0.0f, 0.0f, 0.0f, 1.0f);
}

void Light::setSize(float s) {
  size = s;
  // Rebuild immediately so a lone setSize never leaves a stale projection.
  resize(1.0f);
}

void Light::resize(float ratio) {
  projMatrix.setOrthogonal(-size, size, size / ratio, -size / ratio, 60.0,
                           200.0);
  projMatrixCascade.setOrthogonal(-sizeCascade, sizeCascade,
                                  sizeCascade / ratio, -sizeCascade / ratio,
                                  40.0, 300.0);
}

void Light::setDirection(const Vector3 &v) {
  this->direction.copy(v);
}

Vector3 &Light::getDirection() { return direction; }

void Light::lookAt(const Vector3 &target) {
  direction = target - position;
  direction.normalize();

  viewMatrix.lookAt2(position, target, Vector3::Y);
}

Vector3 &Light::getIntensity() { return intensity; }

void Light::setIntensity(const Vector3 &v) {
  this->intensity.copy(v);
}

Vector3 &Light::getPosition() { return position; }

void Light::setPosition(const Vector3 &v) {
  this->position.copy(v);
}

Matrix4 &Light::getProjMatrix() { return projMatrix; }

Matrix4 &Light::getProjMatrixCascade() { return projMatrixCascade; }

Matrix4 &Light::getViewMatrix() { return viewMatrix; }

Matrix4 &Light::getTextureProjMatrix(const Matrix4 &modelMatrix) {
  textureProjMatrix = biasMatrix * projMatrix * viewMatrix * modelMatrix;
#ifndef NDEBUG
  // Ortho-only invariant: perspective divide must be a no-op (w == 1).
  // A perspective light would break this and needs divide-by-w here.
  assert(textureProjMatrix.data[3][0] == 0.0f);
  assert(textureProjMatrix.data[3][1] == 0.0f);
  assert(textureProjMatrix.data[3][2] == 0.0f);
  assert(textureProjMatrix.data[3][3] == 1.0f);
#endif
  return textureProjMatrix;
}

Matrix4 &Light::getTextureProjMatrixCascade(const Matrix4 &modelMatrix) {
  textureProjMatrixCascade =
      biasMatrix * projMatrixCascade * viewMatrix * modelMatrix;
#ifndef NDEBUG
  assert(textureProjMatrixCascade.data[3][0] == 0.0f);
  assert(textureProjMatrixCascade.data[3][1] == 0.0f);
  assert(textureProjMatrixCascade.data[3][2] == 0.0f);
  assert(textureProjMatrixCascade.data[3][3] == 1.0f);
#endif
  return textureProjMatrixCascade;
}
