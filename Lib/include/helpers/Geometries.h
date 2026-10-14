#pragma once

#include "core/Geometry.h"

class Geometries
{
public:
	static Geometry* cube(float size = 1.0);
	static Geometry* quad(float size);
	static Geometry* point();
};