#pragma once

#include "mesh.h"
#include "materials/materialskybox.h"

class SkyBox : public Mesh {
public:
  SkyBox(MaterialSkyBox *mat);
};