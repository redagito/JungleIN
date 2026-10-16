#pragma once

#include "objects/Mesh.h"
#include "materials/MaterialSkybox.h"

class SkyBox : public Mesh {
public:
  // Takes ownership of the material.
  SkyBox(std::unique_ptr<MaterialSkyBox> mat);
};