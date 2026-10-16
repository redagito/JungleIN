#include <array>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <filesystem>
#include <string>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include "TestFiles.h"
#include "objects/Object3D.h"
#include "objects/Path.h"

using Catch::Matchers::WithinAbs;
using TestFiles::WriteBMP;

namespace {
// 2x2 mask: top row black (walkable), bottom row white (blocked).
// Rows are top-down: rowsTopDown[0] is the top row of the file.
std::filesystem::path WriteMask(const std::string &name) {
  const std::vector<std::array<unsigned char, 3>> mask = {
      {0, 0, 0}, {0, 0, 0}, {255, 255, 255}, {255, 255, 255}};
  const auto file = std::filesystem::temp_directory_path() / name;
  WriteBMP(file, 2, 2, mask);
  return file;
}
} // namespace

// Path::validate takes explicit map bounds instead of reading the
// mutable Ground::MAP_MIN/MAX statics, so tests (and callers) pin the
// mapping they mean. validate() maps bound-min to pixel row/col 0.
TEST_CASE("Path validate takes explicit bounds", "[objects][path][quickwins]")
{
  const auto file = WriteMask("junglein_quickwins_path.bmp");
  const Path path(file.string());

  // Bounds (-120, 120): (-60,-60) samples pixel (0,0): black -> walkable.
  CHECK(path.validate(-60.0, -60.0, -120.0, 120.0));
  // (-60,60) samples pixel (0,1): white -> blocked.
  CHECK_FALSE(path.validate(-60.0, 60.0, -120.0, 120.0));

  // Lower edge maps to pixel 0 (in range); the upper edge maps to pixel
  // width/height, which is out of range. Note the historical truncation
  // toward zero: a negative overflow smaller than one pixel still lands
  // on pixel 0, so out-of-range probes use a full pixel of margin.
  CHECK(path.validate(-120.0, -120.0, -120.0, 120.0));
  CHECK_FALSE(path.validate(120.0, 120.0, -120.0, 120.0));
  CHECK_FALSE(path.validate(-250.0, -120.0, -120.0, 120.0));
  CHECK_FALSE(path.validate(0.0, -250.0, -120.0, 120.0));

  // The bounds are actually used: with (-60,60) the point (-150,-90)
  // is off-map, while the old +-120 statics would have mapped it onto
  // the walkable pixel (0,0).
  CHECK_FALSE(path.validate(-150.0, -90.0, -60.0, 60.0));
  CHECK(path.validate(-30.0, -30.0, -60.0, 60.0));

  std::filesystem::remove(file);
}

// Object3D rotation convention lock: updateMatrix for single-axis
// 90-degree euler inputs must match glm::quat(glm::radians(...)).
// The bespoke EulerToQuat negated the Z rotation, so the (0,0,90) case
// pins the fix.
TEST_CASE("Object3D rotation matches glm euler reference",
          "[objects][object3d][quickwins]")
{
  const std::vector<glm::vec3> cases = {
      {90.0f, 0.0f, 0.0f}, {0.0f, 90.0f, 0.0f}, {0.0f, 0.0f, 90.0f}};
  for (const glm::vec3 &degrees : cases) {
    INFO("degrees: " << degrees.x << "," << degrees.y << "," << degrees.z);
    Object3D object;
    object.setRotation(degrees);
    object.setPosition(glm::vec3(0.0f));
    object.setScale(glm::vec3(1.0f));
    const glm::mat4 m = object.getMatrix();
    const glm::mat3 expected =
        glm::mat3_cast(glm::quat(glm::radians(degrees)));
    for (int c = 0; c < 3; ++c)
      for (int r = 0; r < 3; ++r)
        CHECK_THAT(m[c][r], WithinAbs(expected[c][r], 1e-5f));
  }
}
