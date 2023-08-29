#pragma once

#include <string>

#include "core/Image.h"

class Path {
public:
  Path(std::string file);
  bool validate(double x, double z);

private:
  int width, height;
  Image image;
};