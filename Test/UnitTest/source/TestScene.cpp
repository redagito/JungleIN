#include <catch2/catch_test_macros.hpp>
#include <stdexcept>

#include "objects/Scene.h"

// An empty Scene must fail fast with context on init/update instead of
// dereferencing null sky/ground/camera, and must start non-renderable.
TEST_CASE("Scene fails fast when incomplete", "[objects][scene]")
{
  SECTION("starts non-renderable")
  {
    Scene scene;
    CHECK(scene.getRenderAble() == false);
  }

  SECTION("init without sky, sun and ground throws")
  {
    Scene scene;
    CHECK_THROWS_AS(scene.init(), std::runtime_error);
  }

  SECTION("update without ground and camera throws")
  {
    Scene scene;
    CHECK_THROWS_AS(scene.update(1.0f), std::runtime_error);
  }
}
