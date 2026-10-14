#ifndef MATERIALSHADOW_H
#define MATERIALSHADOW_H

#include "materials/Material.h"

class MaterialShadow : public Material {
public:
  MaterialShadow();
  Shader *bind();
};

#endif // MATERIALSHADOW_H
