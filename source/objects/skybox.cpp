#include "skybox.h"
#include "../helpers/geometries.h"
#include <iostream>
SkyBox::SkyBox(MaterialSkyBox *mat) : Mesh() {
  geometry = Geometries::cube();
  material = mat;
}
