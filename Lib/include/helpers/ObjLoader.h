#pragma once

#include <string>

#include "objects/Mesh.h"

class ObjLoader {
public:
	static Mesh* loadObj(std::string fileName);
};