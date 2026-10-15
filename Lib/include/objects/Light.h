#ifndef LIGHT_H
#define LIGHT_H

#include "math/Vector3.h"
#include "math/Matrix4.h"
#include <string>

/*!
 * Light
 * A light defined by a position and an orientation vector
 */
class Light {
public:
  Light(Vector3 position = Vector3(0, 1, 0),
        Vector3 direction = Vector3(0, -1, 0),
        Vector3 intensity = Vector3(1, 1, 1));

  void setDirection(const Vector3 &direction);
  Vector3 &getDirection();
  void lookAt(const Vector3 &target);

  Vector3 &getIntensity();
  void setIntensity(const Vector3 &intensity);

  Vector3 &getPosition();
  void setPosition(const Vector3 &position);

  Matrix4 &getProjMatrix();
  Matrix4 &getProjMatrixCascade();
  Matrix4 &getViewMatrix();
  // Ortho-only: bias*ortho*view*model preserves w == 1 (asserted in
  // debug). Shaders rely on this for textureProj; a perspective light
  // would need an explicit divide-by-w instead.
  Matrix4 &getTextureProjMatrix(const Matrix4 &modelMatrix);
  Matrix4 &getTextureProjMatrixCascade(const Matrix4 &modelMatrix);

  void resize(float ratio);
  void setSize(float s);

private:
  Vector3 intensity;
  Vector3 position;
  Vector3 direction;

  float size, sizeCascade;

  Matrix4 viewMatrix;
  Matrix4 projMatrix;
  Matrix4 biasMatrix;
  Matrix4 textureProjMatrix;
  Matrix4 projMatrixCascade;
  Matrix4 textureProjMatrixCascade;
};

#endif // LIGHT_H
