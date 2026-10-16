#ifndef LIGHT_H
#define LIGHT_H

#include <glm/glm.hpp>

#include <string>

/*!
 * Light
 * A light defined by a position and an orientation vector.
 */
class Light {
public:
  Light(glm::vec3 position = glm::vec3(0.0f, 1.0f, 0.0f),
        glm::vec3 direction = glm::vec3(0.0f, -1.0f, 0.0f),
        glm::vec3 intensity = glm::vec3(1.0f, 1.0f, 1.0f));

  void setDirection(const glm::vec3 &direction);
  glm::vec3 &getDirection();
  const glm::vec3 &getDirection() const;
  void lookAt(const glm::vec3 &target);

  glm::vec3 &getIntensity();
  const glm::vec3 &getIntensity() const;
  void setIntensity(const glm::vec3 &intensity);

  glm::vec3 &getPosition();
  const glm::vec3 &getPosition() const;
  void setPosition(const glm::vec3 &position);

  glm::mat4 &getProjMatrix();
  const glm::mat4 &getProjMatrix() const;
  glm::mat4 &getProjMatrixCascade();
  const glm::mat4 &getProjMatrixCascade() const;
  glm::mat4 &getViewMatrix();
  const glm::mat4 &getViewMatrix() const;
  // Ortho-only: bias*ortho*view*model preserves w == 1 (asserted in
  // debug). Shaders rely on this for textureProj; a perspective light
  // would need an explicit divide-by-w instead.
  glm::mat4 &getTextureProjMatrix(const glm::mat4 &modelMatrix);
  glm::mat4 &getTextureProjMatrixCascade(const glm::mat4 &modelMatrix);

  void resize(float ratio);
  void setSize(float s);

private:
  glm::vec3 intensity;
  glm::vec3 position;
  glm::vec3 direction;

  float size, sizeCascade;

  glm::mat4 viewMatrix;
  glm::mat4 projMatrix;
  glm::mat4 biasMatrix;
  glm::mat4 textureProjMatrix;
  glm::mat4 projMatrixCascade;
  glm::mat4 textureProjMatrixCascade;
};

#endif // LIGHT_H
