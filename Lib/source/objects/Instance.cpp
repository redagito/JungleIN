#include "objects/Instance.h"

#include "objects/Mesh.h"

Instance::Instance(Mesh *mesh, int id) : Object3D() {
  this->id = id;
  this->mesh = mesh;
}

int Instance::getId() { return id; }

Mesh *Instance::getMesh() { return mesh; }
