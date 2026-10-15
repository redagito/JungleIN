#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <glm/glm.hpp>

#include "objects/Object3D.h"

using Catch::Matchers::WithinAbs;

// translateRel moves along a matrix-rotated axis but must still flag the
// matrix stale: otherwise getMatrix keeps returning the pre-move matrix.
TEST_CASE("Object3D translateRel refreshes the matrix", "[objects][object3d]")
{
  Object3D object;
  object.translateRelX(5.0f);
  // Column-major glm stores translation in column [3]; the matrix must
  // reflect the 5 m move after the refresh.
  CHECK_THAT(object.getMatrix()[3][0], WithinAbs(5.0f, 1e-5f));
}

// updateMatrix composes T * R * S: with identity rotation the diagonal
// carries the scale and the translation column carries the position.
TEST_CASE("Object3D updateMatrix composes translate and scale", "[objects][object3d]")
{
  Object3D object;
  object.setPosition(glm::vec3(1.0f, 2.0f, 3.0f));
  object.setScale(glm::vec3(2.0f, 2.0f, 2.0f));
  const glm::mat4 &m = object.getMatrix();
  CHECK_THAT(m[0][0], WithinAbs(2.0f, 1e-5f));
  CHECK_THAT(m[1][1], WithinAbs(2.0f, 1e-5f));
  CHECK_THAT(m[2][2], WithinAbs(2.0f, 1e-5f));
  CHECK_THAT(m[3][0], WithinAbs(1.0f, 1e-5f));
  CHECK_THAT(m[3][1], WithinAbs(2.0f, 1e-5f));
  CHECK_THAT(m[3][2], WithinAbs(3.0f, 1e-5f));
}
