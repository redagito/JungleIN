#pragma once

#include "materials/Material.h"

class MaterialShadow : public Material {
public:
  MaterialShadow();
  Shader *bind();
};
