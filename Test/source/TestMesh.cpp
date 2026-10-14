#include <catch2/catch_test_macros.hpp>
#include <stdexcept>

#include "objects/Mesh.h"

// A default Mesh carries null geometry/material. Touching the GL pipeline
// with it must throw a clear error, never segfault on a null dereference.
TEST_CASE("Mesh fails fast on missing geometry", "[objects][mesh]")
{
  SECTION("initVBO without geometry throws")
  {
    Mesh mesh;
    CHECK_THROWS_AS(mesh.initVBO(), std::runtime_error);
  }

  SECTION("draw without geometry throws")
  {
    Mesh mesh;
    CHECK_THROWS_AS(mesh.drawWithVBO(), std::runtime_error);
  }
}
