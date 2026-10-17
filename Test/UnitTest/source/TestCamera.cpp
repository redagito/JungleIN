#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "objects/Camera.h"

using Catch::Matchers::WithinAbs;

// Terrain is a floor, not a rail: the camera never sinks below
// ground plus eye height, but level flight over falling terrain keeps
// its altitude instead of bobbing with every bump. Mouse deltas must
// be consumed once so the view does not keep spinning after the mouse
// stops.
TEST_CASE("Camera follows terrain and consumes look input", "[objects][camera]")
{
  SECTION("never sinks below ground plus eye height")
  {
    Camera camera;
    camera.reset(); // eye at y = 6
    camera.update(/*groundY=*/100.0, /*delta=*/1.0);
    CHECK_THAT(camera.getPosition().y, WithinAbs(100.0f + 2.2f, 1e-4f));
  }

  SECTION("stays level downhill instead of sticking to terrain")
  {
    Camera camera;
    camera.reset();
    camera.setPositionY(150.0f);
    camera.update(/*groundY=*/100.0, /*delta=*/1.0);
    CHECK_THAT(camera.getPosition().y, WithinAbs(150.0f, 1e-4f));
    // Falling terrain below leaves the altitude alone...
    camera.update(/*groundY=*/0.0, /*delta=*/1.0);
    CHECK_THAT(camera.getPosition().y, WithinAbs(150.0f, 1e-4f));
    // ...while rising terrain lifts the camera (collision floor).
    camera.update(/*groundY=*/200.0, /*delta=*/1.0);
    CHECK_THAT(camera.getPosition().y, WithinAbs(200.0f + 2.2f, 1e-4f));
  }

  SECTION("E climbs freely and ignores terrain changes")
  {
    Camera camera;
    camera.reset(); // y = 6, above the floor for groundY = 0.
    camera.setEKey(true);
    camera.update(/*groundY=*/0.0, /*delta=*/0.5f);
    camera.setEKey(false);
    // Climbs from the current altitude: 6 + 200 * 0.5.
    CHECK_THAT(camera.getPosition().y, WithinAbs(106.0f, 1e-3f));
    // Terrain changing below changes nothing at altitude.
    camera.update(/*groundY=*/0.0, /*delta=*/1.0);
    CHECK_THAT(camera.getPosition().y, WithinAbs(106.0f, 1e-3f));
  }

  SECTION("E respects a ceiling that follows the terrain")
  {
    Camera camera;
    camera.reset();
    camera.setEKey(true);
    camera.update(/*groundY=*/100.0, /*delta=*/10.0);
    camera.setEKey(false);
    CHECK_THAT(camera.getPosition().y,
      WithinAbs(100.0f + 2.2f + 200.0f, 1e-3f));
  }

  SECTION("Q descends to the floor, never below it")
  {
    Camera camera;
    camera.reset();
    camera.setPositionY(150.0f);
    camera.setQKey(true);
    camera.update(/*groundY=*/100.0, /*delta=*/0.1f);
    camera.setQKey(false);
    // Free descent while above the floor: 150 - 200 * 0.1.
    CHECK_THAT(camera.getPosition().y, WithinAbs(130.0f, 1e-3f));
    // Down to the floor, then the floor stops further descent.
    camera.setQKey(true);
    camera.update(/*groundY=*/100.0, /*delta=*/1.0);
    camera.setQKey(false);
    CHECK_THAT(camera.getPosition().y, WithinAbs(100.0f + 2.2f, 1e-3f));
    // Pressing Q on the floor changes nothing.
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

  SECTION("mouse motion while not looking causes no kick on enable")
  {
    constexpr float kFiveDegrees = 5.0f * 3.141592653f / 180.0f;
    Camera camera;
    camera.reset();
    camera.setMouse(0, 0);
    camera.update(0.0, 1.0);
    // Big motion while look is off must be forgotten, not queued.
    camera.setMouse(1000, 500);
    camera.update(0.0, 1.0);
    camera.activeLook(true);
    camera.update(0.0, 1.0);
    CHECK_THAT(camera.getYaw(), WithinAbs(0.0f, 1e-5f));
    // Fresh motion after enabling still turns exactly once.
    camera.setMouse(1010, 500);
    camera.update(0.0, 1.0);
    CHECK_THAT(camera.getYaw(), WithinAbs(kFiveDegrees, 1e-5f));
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

  SECTION("target round-trips through the glm API")
  {
    Camera camera;
    camera.setTarget(glm::vec3(1.0f, 2.0f, 3.0f));
    const glm::vec3 target = camera.getTarget();
    CHECK_THAT(target.x, WithinAbs(1.0f, 1e-5f));
    CHECK_THAT(target.y, WithinAbs(2.0f, 1e-5f));
    CHECK_THAT(target.z, WithinAbs(3.0f, 1e-5f));
  }
}
