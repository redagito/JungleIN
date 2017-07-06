#include "path.h"
#include "ground.h"

#include <iostream>

Path::Path(std::string file) {
  // load the heightmap picture
  image = Image(file);
  // init the size of the loaded picture
  width = image.getWidth();
  height = image.getHeight();
}

// compute if the position x, z is contained in the path.
bool Path::validate(double x, double z) {
  float range = Ground::MAP_MAX - Ground::MAP_MIN;
  int offset = -Ground::MAP_MIN;
  float xr = (x + offset) / range;
  float yr = (z + offset) / range;
  int xp = xr * width;
  int yp = yr * height;

  if (xp < 0 || xp >= width || yp < 0 || yp >= height) {
    return false;
  } else if (image.getPixel(xp, yp).r > 0) {
    return false;
  } else {
    return true;
  }
}
