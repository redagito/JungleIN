#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

#include <glm/glm.hpp>

#include "core/Image.h"
#include "helpers/ObjLoader.h"
#include "helpers/Timer.h"
#include "helpers/Utils.h"
#include "objects/Camera.h"
#include "objects/Ground.h"
#include "objects/Light.h"

using Catch::Matchers::WithinAbs;

namespace {

std::filesystem::path WriteTempObj(const std::string &name,
                                   const std::string &body) {
  const auto path = std::filesystem::temp_directory_path() / name;
  std::ofstream out(path, std::ios::binary);
  out << body;
  out.close();
  return path;
}

} // namespace

// Item 1: Utils::value must throw on a missing key instead of log-inserting
// a default (which silently mutates the caller's map).
TEST_CASE("Utils::value fails fast on missing keys", "[helpers][utils]")
{
  std::unordered_map<std::string, int> data{{"a", 1}};
  CHECK(value(data, std::string("a")) == 1);
  CHECK_THROWS_AS(value(data, std::string("missing")), std::runtime_error);
  CHECK(data.find("missing") == data.end());
  CHECK(data.size() == 1);
}

// Item 2: Ground::getY divided by n.y unguarded and returned a magic
// MAX_HEIGHT/2 off-map. A full Ground fixture is infeasible headless:
// the constructor needs a MaterialGround, which needs a GL-registered
// shader (ShaderLibrary::getShader throws without one) and GL textures
// (Texture::~Texture calls glDeleteTextures). The guard therefore lives
// in the extracted Ground::planeHeightAt, which getY calls, and the
// contract is pinned here: vertical normal throws, off-map throws,
// ordinary planes interpolate.
TEST_CASE("Ground::planeHeightAt fails fast", "[objects][ground]")
{
  SECTION("flat plane interpolates to its own height")
  {
    CHECK_THAT(Ground::planeHeightAt(glm::vec3(0.0f, 2.0f, 0.0f),
                                     glm::vec3(0.0f, 1.0f, 0.0f), 10.0f,
                                     -20.0f),
               WithinAbs(2.0f, 1e-5f));
  }

  SECTION("tilted plane interpolates along the slope")
  {
    // Plane through the origin with normal (1,1,0)/sqrt(2): y = -x.
    const glm::vec3 n = glm::normalize(glm::vec3(1.0f, 1.0f, 0.0f));
    CHECK_THAT(
        Ground::planeHeightAt(glm::vec3(0.0f), n, 3.0f, 0.0f),
        WithinAbs(-3.0f, 1e-4f));
  }

  SECTION("vertical normal throws instead of dividing by zero")
  {
    CHECK_THROWS_AS(Ground::planeHeightAt(glm::vec3(0.0f),
                                          glm::vec3(1.0f, 0.0f, 0.0f),
                                          0.0f, 0.0f),
                    std::runtime_error);
    CHECK_THROWS_AS(Ground::planeHeightAt(glm::vec3(0.0f),
                                          glm::vec3(0.0f),
                                          0.0f, 0.0f),
                    std::runtime_error);
  }

  SECTION("off-map coordinates throw instead of a magic height")
  {
    const glm::vec3 p(0.0f);
    const glm::vec3 n(0.0f, 1.0f, 0.0f);
    CHECK_THROWS_AS(Ground::planeHeightAt(p, n, 450.0f, 0.0f),
                    std::out_of_range);
    CHECK_THROWS_AS(Ground::planeHeightAt(p, n, -450.0f, 0.0f),
                    std::out_of_range);
    CHECK_THROWS_AS(Ground::planeHeightAt(p, n, 0.0f, 500.0f),
                    std::out_of_range);
    CHECK_THROWS_AS(Ground::planeHeightAt(p, n, 1000.0f, 1000.0f),
                    std::out_of_range);
  }
}
// Item 3: Light::lookAt with target == position must throw instead of
// keeping a stale viewMatrix (and zeroing the direction).
TEST_CASE("Light::lookAt fails fast on degenerate target", "[objects][light]")
{
  Light light(glm::vec3(1.0f, 2.0f, 3.0f));
  const glm::vec3 dirBefore = light.getDirection();
  CHECK_THROWS_AS(light.lookAt(glm::vec3(1.0f, 2.0f, 3.0f)),
                  std::runtime_error);
  // Fail fast means no partial state change.
  CHECK(light.getDirection() == dirBefore);
  // A normal lookAt still works afterwards.
  light.lookAt(glm::vec3(1.0f, 1.0f, 3.0f));
  CHECK_THAT(light.getDirection().x, WithinAbs(0.0f, 1e-6f));
  CHECK_THAT(light.getDirection().y, WithinAbs(-1.0f, 1e-6f));
  CHECK_THAT(light.getDirection().z, WithinAbs(0.0f, 1e-6f));
}

// Item 4: Timer must keep fractional seconds on a steady clock: rapid
// restart() calls sum without loss, and a busy loop is measurable.
TEST_CASE("Timer keeps fractional seconds on a steady clock",
          "[helpers][timer]")
{
  Timer timer;
  timer.start();
  double sum = 0.0;
  for (int i = 0; i < 200; ++i)
    sum += timer.restart();
  CHECK(sum >= 0.0);

  timer.start();
  volatile unsigned long long sink = 0;
  for (unsigned long long i = 0; i < 100000ULL; ++i)
    sink += i;
  CHECK(timer.restart() > 0.0);

  timer.start();
  for (int i = 0; i < 1000; ++i)
    CHECK(timer.restart() >= 0.0);
}

// Item 5: Camera::reset must clear every input bit (move keys, fly keys,
// sprint, look, pending mouse deltas), so update() with no new input is
// a no-op for position and orientation.
TEST_CASE("Camera::reset clears all input state", "[objects][camera]")
{
  Camera camera;
  camera.reset();
  camera.setUpKey(true);
  camera.setSprintKey(true);
  camera.setEKey(true);
  camera.activeLook(true);
  camera.setMouse(0, 0);
  camera.update(0.0, 1.0);
  camera.setMouse(10, 5);
  camera.reset();

  camera.update(0.0, 1.0);
  const glm::vec3 pos = camera.getPosition();
  CHECK_THAT(pos.x, WithinAbs(0.0f, 1e-5f));
  CHECK_THAT(pos.z, WithinAbs(-1.0f, 1e-5f));
  CHECK_THAT(pos.y, WithinAbs(2.2f, 1e-5f));
  CHECK_THAT(camera.getYaw(), WithinAbs(0.0f, 1e-6f));

  // Sprint was cleared: fresh forward input walks at moveSpeed, not double.
  camera.setUpKey(true);
  camera.update(0.0, 1.0);
  CHECK_THAT(camera.getPosition().z, WithinAbs(-1.0f - 200.0f, 1e-3f));

  // Pending mouse deltas were cleared: a fresh look does not kick.
  camera.setUpKey(false);
  camera.activeLook(true);
  camera.update(0.0, 1.0);
  CHECK_THAT(camera.getYaw(), WithinAbs(0.0f, 1e-6f));
}

// Item 7: ObjLoader::loadObj has a single contract - throw on error.
// Probe of malformed inputs plus a valid file that must parse.
TEST_CASE("ObjLoader::loadObj fails fast on malformed files",
          "[helpers][objloader]")
{
  const auto garbage =
      WriteTempObj("junglein_bad.obj", "this is not an obj file\n@@@\n");
  CHECK_THROWS_AS(ObjLoader::loadObj(garbage.string()), std::runtime_error);

  const auto empty = WriteTempObj("junglein_empty.obj", "");
  CHECK_THROWS_AS(ObjLoader::loadObj(empty.string()), std::runtime_error);

  const auto missing =
      std::filesystem::temp_directory_path() / "junglein_does_not_exist.obj";
  std::filesystem::remove(missing);
  CHECK_THROWS_AS(ObjLoader::loadObj(missing.string()), std::runtime_error);

  // A parse that yields no geometry (vertices but no faces) is an asset
  // bug: it must throw here, not hand a hollow Mesh to the GL pipeline.
  const auto nofaces =
      WriteTempObj("junglein_nofaces.obj", "v 0 0 0\nv 1 0 0\nv 0 1 0\n");
  try {
    auto hollow = ObjLoader::loadObj(nofaces.string());
    FAIL("vertices-only OBJ loaded without error");
  } catch (const std::runtime_error &e) {
    CHECK(std::string(e.what()).find("no geometry") != std::string::npos);
  }

  // A valid file must parse. Headless the ShaderLibrary holds no
  // "rt_basic" (registering one needs GL), so only the shader-lookup
  // stage may fail here - never the OBJ parse itself.
  const auto valid = WriteTempObj("junglein_good.obj",
                                  "v 0 0 0\nv 1 0 0\nv 0 1 0\nvn 0 0 1\n"
                                  "vt 0 0\nvt 1 0\nvt 0 1\n"
                                  "f 1/1/1 2/2/1 3/3/1\n");
  try {
    auto mesh = ObjLoader::loadObj(valid.string());
  } catch (const std::runtime_error &e) {
    CHECK(std::string(e.what()).find("Failed to load obj") ==
          std::string::npos);
  }

  std::filesystem::remove(garbage);
  std::filesystem::remove(empty);
  std::filesystem::remove(nofaces);
  std::filesystem::remove(valid);
}

// Item 8a: Image::load must throw on failure (single contract with the
// constructor, which already throws).
TEST_CASE("Image::load fails fast", "[core][image]")
{
  Image img;
  CHECK_THROWS_AS(img.load("does-not-exist-junglein.png", true),
                  std::runtime_error);
  CHECK_THROWS_AS(Image("does-not-exist-junglein.png"), std::runtime_error);
}

// Item 8b: Ground construction with a bad heightmap path must throw.
// (Leak-freedom of the Path allocation is by construction - see the
// reorder in Ground.cpp - since no sanitizer is wired into the presets.)
TEST_CASE("Ground construction fails fast on bad heightmap",
          "[objects][ground]")
{
  CHECK_THROWS_AS(Ground("does-not-exist-junglein-heightmap.png", nullptr),
                  std::runtime_error);
}

// Item 9: the Renderer delta formula (elapsed seconds / TARGET_FPS,
// co-tuned with Camera::moveSpeed) is pinned through the extracted pure
// helper. Renderer.cpp itself stays on its inline formula: that file is
// coordinator-owned, so adopting movementDelta there is follow-up work.
TEST_CASE("Utils::movementDelta pins the Renderer delta formula",
          "[helpers][utils]")
{
  const double saved = Utils::TARGET_FPS.load();
  Utils::TARGET_FPS.store(32.0);
  CHECK_THAT(Utils::movementDelta(1.0), WithinAbs(1.0 / 32.0, 1e-12));
  CHECK_THAT(Utils::movementDelta(0.5), WithinAbs(0.5 / 32.0, 1e-12));
  CHECK_THAT(Utils::movementDelta(0.0), WithinAbs(0.0, 1e-12));
  Utils::TARGET_FPS.store(60.0);
  CHECK_THAT(Utils::movementDelta(1.0), WithinAbs(1.0 / 60.0, 1e-12));
  Utils::TARGET_FPS.store(saved);
}

// Items 6 and 10 need no headless test in this file: item 6 is a
// constant-naming-only change locked by the existing suite, and item 10
// (rt_shadow alphaCutoff uniform + transmit) is covered by the "Shaders
// compile and link" test, which compiles every shader pair.
