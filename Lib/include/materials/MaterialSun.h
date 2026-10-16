#ifndef MATERIAL_SUN_H
#define MATERIAL_SUN_H

#include "materials/Material.h"
#include "core/Texture.h"

class MaterialSun : public Material {
public:
  MaterialSun(Texture *diffuse, Texture *alpha);
  ~MaterialSun() override;
  Shader *bind();

protected:
  Texture *diffuse = nullptr;
  Texture *alpha = nullptr;
};

#endif // MATERIAL_SUN_H
