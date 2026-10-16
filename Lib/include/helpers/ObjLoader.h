#pragma once

#include <memory>
#include <string>
#include <vector>

#include <tiny_obj_loader.h>

#include "helpers/FlattenedMesh.h"
#include "objects/Mesh.h"

class ObjLoader {
public:
  static std::unique_ptr<Mesh> loadObj(std::string fileName);

  using FlattenedMesh = ::FlattenedMesh;

  // Expands per-corner indices into flat buffers. Throws
  // std::runtime_error naming the source and face on the first
  // invalid index instead of dying later with a bare out_of_range.
  static FlattenedMesh flatten(const tinyobj::attrib_t& attrib,
                               const std::vector<tinyobj::shape_t>& shapes,
                               const std::string& source);
};