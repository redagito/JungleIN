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

  SECTION("sticks to terrain downhill, not just uphill")
  {
    Camera camera;
    camera.reset();
    camera.setPositionY(150.0f);
    camera.update(/*groundY=*/100.0, /*delta=*/1.0);
    CHECK_THAT(camera.getPosition().y, WithinAbs(100.0f + 2.2f, 1e-4f));
  }

  SECTION("E raises a persistent offset that follows terrain")
  {
    Camera camera;
    camera.reset();
    camera.setEKey(true);
    camera.update(/*groundY=*/100.0, /*delta=*/0.5f);
    camera.setEKey(false);
    // offset = 200 * 0.5 = 100 above the eye height...
    CHECK_THAT(camera.getPosition().y, WithinAbs(100.0f + 2.2f + 100.0f, 1e-3f));
    // ...and the offset survives terrain changes instead of floating.
    camera.update(/*groundY=*/50.0, /*delta=*/1.0);
    CHECK_THAT(camera.getPosition().y, WithinAbs(50.0f + 2.2f + 100.0f, 1e-3f));
  }

  SECTION("Q lowers back to terrain, never below it")
  {
    Camera camera;
    camera.reset();
    camera.setEKey(true);
    camera.update(/*groundY=*/100.0, /*delta=*/0.5f);
    camera.setEKey(false);
    camera.setQKey(true);
    camera.update(/*groundY=*/100.0, /*delta=*/0.5f);
    camera.setQKey(false);
    CHECK_THAT(camera.getPosition().y, WithinAbs(100.0f + 2.2f, 1e-3f));
    // Pressing Q on the ground changes nothing.
    camera.setQKey(true);
    camera.update(/*groundY=*/100.0, /*delta=*/1.0);
    camera.setQKey(false);
    CHECK_THAT(camera.getPosition().y, WithinAbs(100.0f + 2.2f, 1e-3f));
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

  SECTION("pitch never rolls over past vertical")
  {
    constexpr float kLimit = 89.0f * 3.141592653f / 180.0f;
    Camera camera;
    camera.reset();
    camera.activeLook(true);
    // Stare down: huge downward motion must stop at +89 degrees.
    camera.setMouse(0, 0);
    camera.update(0.0, 1.0);
    camera.setMouse(0, 10000);
    camera.update(0.0, 1.0);
    CHECK_THAT(camera.getPitch(), WithinAbs(kLimit, 1e-5f));
    // Stare up: huge upward motion must stop at -89 degrees.
    camera.setMouse(0, 10000);
    camera.update(0.0, 1.0);
    camera.setMouse(0, -20000);
    camera.update(0.0, 1.0);
    CHECK_THAT(camera.getPitch(), WithinAbs(-kLimit, 1e-5f));
  }
  SECTION("sprint doubles horizontal displacement")
  {
    Camera walk;
    walk.reset();
    walk.setUpKey(true);
    walk.update(/*groundY=*/0.0, /*delta=*/1.0);
    const float walkZ = walk.getPosition().z;

    Camera run;
    run.reset();
    run.setUpKey(true);
    run.setSprintKey(true);
    run.update(/*groundY=*/0.0, /*delta=*/1.0);
    const float runZ = run.getPosition().z;

    // Spawn z is -1, forward decreases z; sprint must cover twice the distance.
    CHECK_THAT((walkZ + 1.0f) * 2.0f, WithinAbs(runZ + 1.0f, 1e-3f));
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
