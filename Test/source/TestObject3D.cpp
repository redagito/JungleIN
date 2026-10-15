#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "objects/Object3D.h"

using Catch::Matchers::WithinAbs;

// translateRel moves along a matrix-rotated axis but must still flag the
// matrix stale: otherwise getMatrix keeps returning the pre-move matrix.
TEST_CASE("Object3D translateRel refreshes the matrix", "[objects][object3d]")
{
  Object3D object;
  object.translateRelX(5.0f);
  CHECK_THAT(object.getMatrix().data[0][3], WithinAbs(5.0f, 1e-5f));
}
