#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <stdexcept>

#include <glm/glm.hpp>

#include "helpers/Geometries.h"
#include "objects/Mesh.h"
#include "render/Shader.h"

using Catch::Matchers::WithinAbs;

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

  // Instanced entry points fail the same way, before any GL call (null
  // shader and missing VAO are checked before touching the driver).
  SECTION("instanced upload without shader or VBO throws")
  {
    Mesh mesh;
    const glm::mat4 model(1.0f);
    CHECK_THROWS_AS(mesh.uploadInstances(&model, 1, nullptr),
      std::runtime_error);
    Shader probe("junglein-mesh-probe");
    CHECK_THROWS_AS(mesh.uploadInstances(&model, 1, &probe),
      std::runtime_error);
  }

  SECTION("instanced draw without geometry throws")
  {
    Mesh mesh;
    CHECK_THROWS_AS(mesh.drawInstanced(1), std::runtime_error);
  }
}

// Bounding box is the min/max over the vertices, computed once and
// cached: the unit quad spans x/y in [-1, 1] at z = 0, the unit cube
// fills [-1, 1] on all axes. Pure CPU over geometry data, no GL.
TEST_CASE("Mesh boundingBox covers its vertices", "[objects][mesh]")
{
  Mesh quad(Geometries::quad(1.0f), nullptr);
  const ModelBox quadBox = quad.boundingBox();
  CHECK_THAT(quadBox.min.x, WithinAbs(-1.0f, 1e-6f));
  CHECK_THAT(quadBox.min.y, WithinAbs(-1.0f, 1e-6f));
  CHECK_THAT(quadBox.min.z, WithinAbs(0.0f, 1e-6f));
  CHECK_THAT(quadBox.max.x, WithinAbs(1.0f, 1e-6f));
  CHECK_THAT(quadBox.max.y, WithinAbs(1.0f, 1e-6f));
  CHECK_THAT(quadBox.max.z, WithinAbs(0.0f, 1e-6f));

  Mesh cube(Geometries::cube(1.0f), nullptr);
  const ModelBox cubeBox = cube.boundingBox();
  CHECK_THAT(cubeBox.min.x, WithinAbs(-1.0f, 1e-6f));
  CHECK_THAT(cubeBox.max.y, WithinAbs(1.0f, 1e-6f));
  CHECK_THAT(cubeBox.min.z, WithinAbs(-1.0f, 1e-6f));
  CHECK_THAT(cubeBox.max.z, WithinAbs(1.0f, 1e-6f));

  Mesh empty;
  CHECK_THROWS_AS(empty.boundingBox(), std::runtime_error);
}
