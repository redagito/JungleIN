#ifndef OBJLOADER_H
#define OBJLOADER_H

#include <string>

#include "../objects/mesh.h"

class ObjLoader {
public:
  static Mesh *loadObj(std::string fileName);
};

#endif // OBJLOADER_H
