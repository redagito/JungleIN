#pragma once

#include <string>

#include "objects/mesh.h"

class ObjLoader {
public:
	static Mesh* loadObj(std::string fileName);
};