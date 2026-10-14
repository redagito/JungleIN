#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "objects/Camera.h"

using Catch::Matchers::WithinAbs;

// Walking must keep the camera above the terrain: the original game set
// every frame. Mouse deltas must be consumed once so the view does not
// keep spinning after the mouse stops.
TEST_CASE("Camera follows terrain and consumes look input", "[objects][camera]")
{
  SECTION("never sinks below ground plus eye height")
  {
    Camera camera;
    camera.reset(); // eye at y = 6
    camera.update(/*groundY=*/100.0, /*delta=*/1.0);
    CHECK_THAT(camera.getPosition().y, WithinAbs(100.0f + 2.2f, 1e-4f));
  }

  SECTION("keeps altitude when already above ground")
  {
    Camera camera;
    camera.reset();
    camera.setPositionY(150.0f);
    camera.update(/*groundY=*/100.0, /*delta=*/1.0);
    CHECK_THAT(camera.getPosition().y, WithinAbs(150.0f, 1e-4f));
  }

  SECTION("mouse deltas turn the view once while looking")
  {
    Camera camera;
    camera.reset();
    camera.activeLook(true);
    camera.setMouse(0, 0);
    camera.update(0.0, 1.0);
    const float yawBefore = camera.getYaw();
    camera.setMouse(10, 0);
    camera.update(0.0, 1.0);
    // lookSpeed 0.5 degrees per pixel: 10 px -> 5 degrees.
    CHECK_THAT(camera.getYaw() - yawBefore, WithinAbs(5.0f * 3.141592653f / 180.0f, 1e-5f));
    // No new mouse input: the view must not keep turning.
    camera.update(0.0, 1.0);
    CHECK_THAT(camera.getYaw() - yawBefore, WithinAbs(5.0f * 3.141592653f / 180.0f, 1e-5f));
  }

  SECTION("mouse deltas are ignored while not looking")
  {
    Camera camera;
    camera.reset();
    camera.setMouse(0, 0);
    camera.update(0.0, 1.0);
    camera.setMouse(50, 20);
    camera.update(0.0, 1.0);
    CHECK_THAT(camera.getYaw(), WithinAbs(0.0f, 1e-5f));
  }
}
