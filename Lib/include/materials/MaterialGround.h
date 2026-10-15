#ifndef MATERIALGROUND_H
#define MATERIALGROUND_H

#include "materials/Material.h"

class MaterialGround : public Material {
public:
  MaterialGround(Texture *moss, Texture *earth, Texture *shatter,
                 Texture *mossNormal, Texture *earthNormal,
                 Texture *shatterNormal);
  ~MaterialGround() override;

  Shader *bind();

  void setHeightRange(float f);
  void setAlphaCutoff(float f);

protected:
  float size = 0.f;
  float heightRange = 0.f;
  float alphaCutoff = 0.8f;

  Texture *moss = nullptr;
  Texture *earth = nullptr;
  Texture *shatter = nullptr;
  Texture *mossNormal = nullptr;
  Texture *earthNormal = nullptr;
  Texture *shatterNormal = nullptr;
};

#endif // MATERIALGROUND_H
