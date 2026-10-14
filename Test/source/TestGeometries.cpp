#include <catch2/catch_test_macros.hpp>

#include "helpers/Geometries.h"

// A closed cube needs 6 faces = 12 triangles = 36 indices; the skybox
// samples all six cubemap faces, so a short count drops the top/bottom.
TEST_CASE("Geometries produce complete primitives", "[helpers][geometries]")
{
  SECTION("cube has all six faces")
  {
    Geometry *cube = Geometries::cube();
    CHECK(cube->getVerticesCount() == 24);
    CHECK(cube->getIndicesCount() == 36);
    const unsigned int *indices = cube->getIndices();
    for (int i = 0; i < cube->getIndicesCount(); ++i)
      CHECK(indices[i] < 8u);
    delete cube;
  }

  SECTION("quad has both triangles")
  {
    Geometry *quad = Geometries::quad(1.0);
    CHECK(quad->getVerticesCount() == 12);
    CHECK(quad->getIndicesCount() == 6);
    CHECK(quad->hasTexCoords());
    delete quad;
  }
}
