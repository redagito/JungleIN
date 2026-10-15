#ifndef SUN_H
#define SUN_H

#include <glm/glm.hpp>

#include "objects/Mesh.h"
#include "objects/Light.h"
#include "materials/MaterialSun.h"
#include "helpers/Geometries.h"

class Sun : public Mesh {

public:
  Sun(MaterialSun *mat);
  ~Sun();

  void update(const glm::vec3 &positionCamera, const glm::vec3 &zCamera);

  void setSpeed(float s);
  Light *getLight();

private:
  Light *light;
  float lightDistance;
  float lightTargetOffset;
  float sunDistance;
  glm::vec3 localPosition;
  float yaw;
  float speed;
};

#endif // SUN_H
