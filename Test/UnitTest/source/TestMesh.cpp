#include <catch2/catch_test_macros.hpp>
#include <stdexcept>

#include <glm/glm.hpp>

#include "objects/Mesh.h"
#include "render/Shader.h"

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
