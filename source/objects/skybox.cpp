#include "skybox.h"
#include "../helpers/geometries.h"
#include <fmtlog/fmtlog.h>
SkyBox::SkyBox(MaterialSkyBox* mat) : Mesh() {
	geometry = Geometries::cube();
	material = mat;
}
