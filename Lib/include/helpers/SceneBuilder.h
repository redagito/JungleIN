#pragma once

#include <memory>

#include "objects/Scene.h"
#include "objects/Ground.h"
#include "helpers/ObjLoader.h"
#include "helpers/Geometries.h"
#include "materials/MaterialBasic.h"
#include "materials/MaterialSun.h"
#include "materials/MaterialGround.h"

class SceneBuilder
{
public:
	// Builds the whole scene; ownership transfers to the caller.
	// Every asset is unique_ptr-owned from birth, so a mid-build throw
	// (missing file, bad image) unwinds cleanly with nothing leaked.
	static std::unique_ptr<Scene> build();
	static void reloadGo(int density, int spacing);
	static Scene* scene2;
	static Ground* ground2;
	static int NB_INSTANCE;

private:
	static void createInstances(Mesh* mesh, Ground* ground, int nbInstance);
};