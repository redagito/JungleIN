#include <catch2/catch_test_macros.hpp>
#include <stdexcept>
#include <string>
#include <vector>

#include <tiny_obj_loader.h>

#include "helpers/ObjLoader.h"

namespace {

tinyobj::attrib_t TriangleAttrib(bool withNormals) {
  tinyobj::attrib_t attrib;
  attrib.vertices = {0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f};
  attrib.texcoords = {0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f};
  if (withNormals)
    attrib.normals = {0.0f, 0.0f, 1.0f};
  return attrib;
}

std::vector<tinyobj::shape_t> TriangleShape(int normalIndex) {
  tinyobj::shape_t shape;
  for (int i = 0; i < 3; ++i) {
    tinyobj::index_t index;
    index.vertex_index = i;
    index.normal_index = normalIndex;
    index.texcoord_index = i;
    shape.mesh.indices.push_back(index);
  }
  return {shape};
}

} // namespace

// The index-flattening must fail fast with context: a bare out_of_range
// from vector::at says nothing about which file or face broke.
TEST_CASE("ObjLoader flattens indices with context", "[helpers][objloader]")
{
  SECTION("valid triangle flattens to indexed soup")
  {
    const auto flat =
        ObjLoader::flatten(TriangleAttrib(true), TriangleShape(0), "test");
    CHECK(flat.vertices.size() == 9);
    CHECK(flat.normals.size() == 9);
    CHECK(flat.uvs.size() == 6);
    CHECK(flat.indices.size() == 3);
    CHECK(flat.vertices[3] == 1.0f);
    CHECK(flat.normals[2] == 1.0f);
  }

  SECTION("missing normals fail fast with context")
  {
    CHECK_THROWS_AS(
        ObjLoader::flatten(TriangleAttrib(false), TriangleShape(-1), "test"),
        std::runtime_error);
  }

  SECTION("out-of-range vertex index fails fast with context")
  {
    auto attrib = TriangleAttrib(true);
    auto shapes = TriangleShape(0);
    shapes[0].mesh.indices[1].vertex_index = 99;
    CHECK_THROWS_AS(ObjLoader::flatten(attrib, shapes, "test"),
                    std::runtime_error);
  }

  SECTION("out-of-range texcoord index fails fast with context")
  {
    auto attrib = TriangleAttrib(true);
    auto shapes = TriangleShape(0);
    shapes[0].mesh.indices[2].texcoord_index = 99;
    CHECK_THROWS_AS(ObjLoader::flatten(attrib, shapes, "test"),
                    std::runtime_error);
  }
}
