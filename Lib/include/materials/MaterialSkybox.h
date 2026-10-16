#ifndef MATERIALSKYBOX_H
#define MATERIALSKYBOX_H

#include <memory>

#include "materials/Material.h"
#include "core/TextureCube.h"

class MaterialSkyBox : public Material {
public:
  // Takes ownership of the cubemap.
  MaterialSkyBox(std::unique_ptr<TextureCube> textureCube);
  ~MaterialSkyBox() override;

  Shader *bind();

private:
  std::unique_ptr<TextureCube> cubemap;
};

#endif // MATERIALSKYBOX_H
