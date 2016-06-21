#ifndef PATH_H
#define PATH_H

#include <string>

#include "core/Image.h"

class Ground;

class Path {
public:
  Path(std::string file);
  bool validate(double x, double z);

private:
  int width, height;
  Image image;
};

#endif // PATH_H
