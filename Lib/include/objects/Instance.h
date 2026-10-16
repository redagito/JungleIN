#pragma once

#include "objects/Object3D.h"

class Mesh;

class Instance : public Object3D {
public:
  Instance(Mesh *mesh, int id = 0);

  int getId();
  Mesh *getMesh();

private:
  int id;
  Mesh *mesh;
};
