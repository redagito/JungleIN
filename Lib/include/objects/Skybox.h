#pragma once

#include "objects/Mesh.h"
#include "materials/MaterialSkybox.h"

class SkyBox : public Mesh {
public:
  SkyBox(MaterialSkyBox *mat);
};