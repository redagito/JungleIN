#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "helpers/Frustum.h"

using Catch::Matchers::WithinAbs;

namespace {

// Unit axis-aligned box helpers: the OBB test takes explicit axes, so
// axis-aligned cases pass the identity basis.
glm::vec3 kAxes[3] = {
    glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f),
    glm::vec3(0.0f, 0.0f, 1.0f)};

} // namespace

// Identity clip matrix is the NDC box: each face sits at +-1, so small
// volumes near the origin are kept and anything past a face is dropped.
TEST_CASE("Frustum extract keeps the NDC box", "[render][frustum]")
{
  const Frustum frustum = Frustum::extract(glm::mat4(1.0f));
  const float extents[3] = {0.1f, 0.1f, 0.1f};

  CHECK(frustum.visibleBox(glm::vec3(0.0f), kAxes, extents));
  // Past each face (left, right, bottom, top, near, far).
  CHECK_FALSE(
      frustum.visibleBox(glm::vec3(-2.0f, 0.0f, 0.0f), kAxes, extents));
  CHECK_FALSE(
      frustum.visibleBox(glm::vec3(2.0f, 0.0f, 0.0f), kAxes, extents));
  CHECK_FALSE(
      frustum.visibleBox(glm::vec3(0.0f, -2.0f, 0.0f), kAxes, extents));
  CHECK_FALSE(
      frustum.visibleBox(glm::vec3(0.0f, 2.0f, 0.0f), kAxes, extents));
  CHECK_FALSE(
      frustum.visibleBox(glm::vec3(0.0f, 0.0f, -2.0f), kAxes, extents));
  CHECK_FALSE(
      frustum.visibleBox(glm::vec3(0.0f, 0.0f, 2.0f), kAxes, extents));
  // Behind the camera means outside the near plane.
  CHECK_FALSE(
      frustum.visibleBox(glm::vec3(0.0f, 0.0f, -1.5f), kAxes, extents));
}

// Straddlers are kept (no edge flicker), fully-outside boxes drop. A
// tall thin box reaching down into view is kept: the case the bounding
// sphere got wrong (its balloon would span the whole height).
TEST_CASE("Frustum box test keeps straddlers", "[render][frustum]")
{
  const Frustum frustum = Frustum::extract(glm::mat4(1.0f));

  // Tree-like: 0.2 wide, 8 tall, spanning y in [0, 8]: the canopy
  // clears the top face but the trunk reaches down into view.
  const float trunk[3] = {0.1f, 4.0f, 0.1f};
  CHECK(frustum.visibleBox(glm::vec3(0.0f, 4.0f, 0.0f), kAxes, trunk));
  // Same footprint, short enough to clear the top face: gone.
  const float stump[3] = {0.1f, 1.0f, 0.1f};
  CHECK_FALSE(
      frustum.visibleBox(glm::vec3(0.0f, 6.0f, 0.0f), kAxes, stump));
  // Rotated 45 degrees about Y at the origin: still fully inside.
  const float angle = glm::radians(45.0f);
  const glm::vec3 turned[3] = {
      glm::vec3(std::cos(angle), 0.0f, -std::sin(angle)),
      glm::vec3(0.0f, 1.0f, 0.0f),
      glm::vec3(std::sin(angle), 0.0f, std::cos(angle))};
  const float small[3] = {0.4f, 0.4f, 0.4f};
  CHECK(frustum.visibleBox(glm::vec3(0.0f), turned, small));
}

// A degenerate clip matrix fails open: culling must never hide the
// scene on bad input.
TEST_CASE("Frustum extract fails open on degenerate input",
  "[render][frustum]")
{
  const Frustum frustum = Frustum::extract(glm::mat4(0.0f));
  const float extents[3] = {1.0f, 1.0f, 1.0f};

  CHECK(frustum.visibleBox(glm::vec3(0.0f), kAxes, extents));
  CHECK(frustum.visibleBox(glm::vec3(100.0f, -50.0f, 25.0f), kAxes,
    extents));
}

// Instance cull decision from a world matrix plus a model box: the box
// rides the matrix (center fully transformed, extents scaled per axis),
// so translation places the volume and scale grows it.
TEST_CASE("CullWorldBox follows the matrix", "[render][frustum]")
{
  const Frustum frustum = Frustum::extract(glm::mat4(1.0f));
  const ModelBox unit = {glm::vec3(-0.5f), glm::vec3(0.5f)};

  // Unit box at the origin stays.
  CHECK_FALSE(CullWorldBox(frustum, glm::mat4(1.0f), unit));
  // Moved past the far face it goes, however small.
  const glm::mat4 far =
      glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 5.0f));
  CHECK(CullWorldBox(frustum, far, unit));
  // A bigger box at the same spot reaches back into view.
  const ModelBox room = {glm::vec3(-5.0f), glm::vec3(5.0f)};
  CHECK_FALSE(CullWorldBox(frustum, far, room));
  // Uniform scale grows the volume: 0.5 * 4 = 2 reaches from z = 2.5.
  const glm::mat4 grown =
      glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 2.5f)) *
      glm::scale(glm::mat4(1.0f), glm::vec3(4.0f));
  CHECK_FALSE(CullWorldBox(frustum, grown, unit));
  CHECK(CullWorldBox(frustum, grown,
    ModelBox{glm::vec3(-0.05f), glm::vec3(0.05f)}));
  // Collapsed (zero-scale) matrix degrades to an exact point test.
  const glm::mat4 flat =
      glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 5.0f)) *
      glm::scale(glm::mat4(1.0f), glm::vec3(0.0f));
  CHECK(CullWorldBox(frustum, flat, unit));
  CHECK_FALSE(CullWorldBox(frustum,
    glm::scale(glm::mat4(1.0f), glm::vec3(0.0f)), unit));
}
