#include "objects/Skybox.h"
#include "helpers/Geometries.h"
#include <fmtlog/fmtlog.h>
SkyBox::SkyBox(MaterialSkyBox* mat) : Mesh() {
	geometry = Geometries::cube();
	material = mat;
}
