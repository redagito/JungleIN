#pragma once

#include <vector>

struct FlattenedMesh {
  std::vector<float> vertices;
  std::vector<float> normals;
  std::vector<float> uvs;
  std::vector<unsigned int> indices;
};
