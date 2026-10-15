#ifndef MATERIALSKYBOX_H
#define MATERIALSKYBOX_H

#include "materials/Material.h"
#include "core/TextureCube.h"

class MaterialSkyBox : public Material {
public:
  MaterialSkyBox(TextureCube *textureCube);
  ~MaterialSkyBox() override;

  Shader *bind();

private:
  TextureCube *cubemap = nullptr;
};

#endif // MATERIALSKYBOX_H
