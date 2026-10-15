#ifndef METERIALSUN_H
#define METERIALSUN_H

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

#endif // METERIALSUN_H
