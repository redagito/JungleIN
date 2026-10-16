#include "objects/Skybox.h"
#include "helpers/Geometries.h"
#include "helpers/Logger.h"
SkyBox::SkyBox(std::unique_ptr<MaterialSkyBox> mat) : Mesh() {
  geometry = Geometries::cube();
	material = std::move(mat);
}
