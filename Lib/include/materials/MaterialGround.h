#ifndef MATERIALGROUND_H
#define MATERIALGROUND_H

#include <memory>

#include "materials/Material.h"

class MaterialGround : public Material {
public:
  // Takes ownership of all six textures.
  MaterialGround(std::unique_ptr<Texture> moss, std::unique_ptr<Texture> earth,
                 std::unique_ptr<Texture> shatter,
                 std::unique_ptr<Texture> mossNormal,
                 std::unique_ptr<Texture> earthNormal,
                 std::unique_ptr<Texture> shatterNormal);
  ~MaterialGround() override;

  Shader *bind();

  void setHeightRange(float f);
  void setAlphaCutoff(float f);
  float getAlphaCutoff() const override { return alphaCutoff; }

protected:
  float size = 0.f;
  float heightRange = 0.f;
  float alphaCutoff = 0.8f;

  std::unique_ptr<Texture> moss;
  std::unique_ptr<Texture> earth;
  std::unique_ptr<Texture> shatter;
  std::unique_ptr<Texture> mossNormal;
  std::unique_ptr<Texture> earthNormal;
  std::unique_ptr<Texture> shatterNormal;
};

#endif // MATERIALGROUND_H
