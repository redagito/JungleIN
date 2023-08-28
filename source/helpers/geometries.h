#pragma once

#include "core/geometry.h"

class Geometries
{
public:
	static Geometry* cube(float size = 1.0);
	static Geometry* quad(float size);
	static Geometry* point();
};