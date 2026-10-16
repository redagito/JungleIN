#pragma once

#include <string>

#include "core/Image.h"

class Path {
public:
  Path(std::string file);
  // Explicit map bounds: world (x, z) in [mapMin, mapMax] maps onto the
  // mask pixels, bound-min to pixel 0. Takes the bounds as parameters
  // so callers (Ground) own the range instead of mutable statics.
  bool validate(double x, double z, double mapMin, double mapMax) const;

private:
  int width, height;
  Image image;
};