#include "objects/Skybox.h"
#include "helpers/Geometries.h"
#include "helpers/Logger.h"
SkyBox::SkyBox(MaterialSkyBox* mat) : Mesh() {
	geometry = Geometries::cube();
	material = mat;
}
