#ifndef METERIALSUN_H
#define METERIALSUN_H

#include "materials/Material.h"
#include "core/Texture.h"

class MaterialSun : public Material {
public:
  MaterialSun(Texture *diffuse, Texture *alpha);
  Shader *bind();

protected:
  Texture *diffuse;
  Texture *alpha;
};

#endif // METERIALSUN_H
