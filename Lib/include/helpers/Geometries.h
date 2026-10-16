#pragma once

#include <memory>

#include "core/Geometry.h"

class Geometries
{
public:
  static std::unique_ptr<Geometry> cube(float size = 1.0);
  static std::unique_ptr<Geometry> quad(float size);
  static std::unique_ptr<Geometry> point();
};