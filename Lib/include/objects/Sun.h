#ifndef SUN_H
#define SUN_H

#include "objects/Mesh.h"
#include "objects/Light.h"
#include "materials/MaterialSun.h"
#include "helpers/Geometries.h"
#include "math/Vector3.h"

class Sun : public Mesh {

public:
  Sun(MaterialSun *mat);

  void update(const Vector3 &positionCamera, const Vector3 &zCamera);

  void setSpeed(float s);
  Light *getLight();

private:
  Light *light;
  float lightDistance;
  float lightTargetOffset;
  float sunDistance;
  Vector3 localPosition;
  float yaw;
  float speed;
};

#endif // SUN_H
