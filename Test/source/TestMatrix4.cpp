#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <cmath>

#include "math/Matrix4.h"
#include "math/Matrix3.h"
#include "math/Vector3.h"

using Catch::Matchers::WithinAbs;

TEST_CASE("Matrix4 identity and multiply", "[math][matrix4]")
{
	SECTION("identity leaves vectors unchanged")
	{
		Matrix4 m;
		m.identity();
		Vector3 v = m * Vector3(1.0f, 2.0f, 3.0f);
		CHECK_THAT(v.x, WithinAbs(1.0, 1e-5));
		CHECK_THAT(v.y, WithinAbs(2.0, 1e-5));
		CHECK_THAT(v.z, WithinAbs(3.0, 1e-5));
	}

	SECTION("operator* composes row-major transforms")
	{
		Matrix4 t;
		t.identity().translate(Vector3(5.0f, 0.0f, 0.0f));
		Matrix4 s;
		s.identity().scale(Vector3(2.0f, 2.0f, 2.0f));
		Matrix4 c = t * s;
		Vector3 v = c * Vector3(1.0f, 0.0f, 0.0f);
		// scale first, then translate: 1*2+5 = 7
		CHECK_THAT(v.x, WithinAbs(7.0, 1e-4));
	}

	SECTION("operator*= keeps the historical reversed order (a *= b == b * a)")
	{
		// NOTE: Matrix4::operator*= is defined as copy(mat * *this), i.e.
		// a *= b computes b * a. This matches the 2012 original source and
		// the whole app depends on it: Camera::update chains
		// translate().rotate() through it and Renderer::pushMatrix composes
		// model matrices through it (camera movement was validated against
		// this behavior). Do NOT "fix" it to textbook order without
		// revalidating camera and scene-graph composition.
		Matrix4 a;
		a.identity().translate(Vector3(1.0f, 2.0f, 3.0f));
		Matrix4 b;
		b.identity().scale(Vector3(2.0f, 2.0f, 2.0f));

		Matrix4 expected = b * a;
		Matrix4 actual(a);
		actual *= b;

		for (int i = 0; i < 16; ++i)
			CHECK_THAT(actual.array[i], WithinAbs(expected.array[i], 1e-5));
	}
}

TEST_CASE("Matrix inverse rejects singular input", "[math][matrix]")
{
  SECTION("zero matrix inverts to zero matrix")
  {
    Matrix4 m;
    m.zero();
    const Matrix4 inverse = m.inverse();
    for (int i = 0; i < 16; ++i)
      CHECK_THAT(inverse.array[i], WithinAbs(0.0f, 1e-6f));
  }

  SECTION("near-singular matrix inverts to zero matrix, not garbage")
  {
    // Third row nearly dependent on the first two: determinant is ~1e-9
    // but the adjugate is O(1), so a naive 1/det blows entries up to ~1e9.
    Matrix4 m;
    m.identity();
    m.data[0][0] = 1.0f;
    m.data[0][1] = 0.0f;
    m.data[0][2] = 0.0f;
    m.data[1][0] = 0.0f;
    m.data[1][1] = 1.0f;
    m.data[1][2] = 0.0f;
    m.data[2][0] = 1.0f;
    m.data[2][1] = 2.0f;
    m.data[2][2] = 1e-9f;
    const Matrix4 inverse = m.inverse();
    for (int i = 0; i < 16; ++i)
      CHECK(std::abs(inverse.array[i]) < 1e6f);
  }

  SECTION("Matrix3 near-singular input falls back to identity")
  {
    Matrix4 from;
    from.identity();
    from.data[0][0] = 1.0f;
    from.data[0][1] = 0.0f;
    from.data[0][2] = 0.0f;
    from.data[1][0] = 0.0f;
    from.data[1][1] = 1.0f;
    from.data[1][2] = 0.0f;
    from.data[2][0] = 1.0f;
    from.data[2][1] = 2.0f;
    from.data[2][2] = 1e-9f;
    Matrix3 result;
    result.getInverse(from);
    for (int i = 0; i < 9; ++i)
      CHECK(std::abs(result.array[i]) < 1e6f);
  }
}

namespace {
float RowLength(const Matrix4 &m, int row) {
  return std::sqrt(m.data[row][0] * m.data[row][0] +
                   m.data[row][1] * m.data[row][1] +
                   m.data[row][2] * m.data[row][2]);
}

float RowDot(const Matrix4 &m, int a, int b) {
  return m.data[a][0] * m.data[b][0] + m.data[a][1] * m.data[b][1] +
         m.data[a][2] * m.data[b][2];
}
} // namespace

TEST_CASE("Matrix4 lookAt2 stays orthonormal", "[math][matrix4]")
{
  SECTION("level view yields identity rotation")
  {
    Matrix4 m;
    m.lookAt2(Vector3(0.0f, 0.0f, 5.0f), Vector3(0.0f, 0.0f, 0.0f),
              Vector3::Y);
    CHECK_THAT(RowLength(m, 0), WithinAbs(1.0f, 1e-5f));
    CHECK_THAT(m.data[0][0], WithinAbs(1.0f, 1e-5f));
    CHECK_THAT(m.data[2][2], WithinAbs(1.0f, 1e-5f));
  }

  SECTION("overhead view does not degenerate")
  {
    Matrix4 m;
    m.lookAt2(Vector3(0.0f, 10.0f, 0.0f), Vector3(0.0f, 0.0f, 0.0f),
              Vector3::Y);
    CHECK_THAT(RowLength(m, 0), WithinAbs(1.0f, 1e-4f));
    CHECK_THAT(RowLength(m, 1), WithinAbs(1.0f, 1e-4f));
    CHECK_THAT(RowLength(m, 2), WithinAbs(1.0f, 1e-4f));
    CHECK_THAT(RowDot(m, 0, 1), WithinAbs(0.0f, 1e-4f));
    CHECK_THAT(RowDot(m, 0, 2), WithinAbs(0.0f, 1e-4f));
    CHECK_THAT(RowDot(m, 1, 2), WithinAbs(0.0f, 1e-4f));
  }
}
