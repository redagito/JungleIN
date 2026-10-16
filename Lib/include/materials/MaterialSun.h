#pragma once

#include <memory>

#include "materials/Material.h"
#include "core/Texture.h"

class MaterialSun : public Material {
public:
  // Takes ownership of both textures.
  MaterialSun(std::unique_ptr<Texture> diffuse, std::unique_ptr<Texture> alpha);
  ~MaterialSun() override;
  Shader *bind();

protected:
  std::unique_ptr<Texture> diffuse;
  std::unique_ptr<Texture> alpha;
};
