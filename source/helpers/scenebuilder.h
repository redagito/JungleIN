#pragma once

#include "render/renderer.h"
#include "objects/scene.h"
#include "objects/ground.h"
#include "helpers/objloader.h"
#include "helpers/geometries.h"
#include "materials/materialbasic.h"
#include "materials/materialsun.h"
#include "materials/materialground.h"

class SceneBuilder 
{
public:
	static Scene* build();
	static void reloadGo(int density, int spacing);
	static Scene* scene2;
	static Ground* ground2;
	static int NB_INSTANCE;

private:
	static void createInstances(Mesh* mesh, Ground* ground, int nbInstance);
};