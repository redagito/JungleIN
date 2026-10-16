#pragma once

#include <memory>

#include <glm/glm.hpp>

#include "objects/Mesh.h"
#include "objects/Light.h"
#include "materials/MaterialSun.h"
#include "helpers/Geometries.h"

class Sun : public Mesh {

public:
  // Takes ownership of the material.
  Sun(std::unique_ptr<MaterialSun> mat);
  ~Sun();

  void update(const glm::vec3 &positionCamera, const glm::vec3 &zCamera);

  void setSpeed(float s);
  // Borrowed: the Sun owns its light.
  Light *getLight();

private:
  std::unique_ptr<Light> light;
  float lightDistance;
  float lightTargetOffset;
  float sunDistance;
  glm::vec3 localPosition;
  float yaw;
  float speed;
};
