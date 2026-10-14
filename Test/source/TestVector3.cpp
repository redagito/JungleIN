#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "math/Vector3.h"

using Catch::Matchers::WithinAbs;

TEST_CASE("Vector3 basics", "[math][vector3]")
{
	SECTION("length and normalize")
	{
		Vector3 v(3.0f, 4.0f, 0.0f);
		CHECK_THAT(v.length(), WithinAbs(5.0, 1e-5));
		v.normalize();
		CHECK_THAT(v.length(), WithinAbs(1.0, 1e-5));
	}

	SECTION("cross product follows right-hand rule")
	{
		Vector3 r;
		r.cross(Vector3::X, Vector3::Y);
		CHECK(r.x == 0.0f);
		CHECK(r.y == 0.0f);
		CHECK(r.z == 1.0f);
	}

	SECTION("dot product")
	{
		Vector3 a(1.0f, 2.0f, 3.0f);
		Vector3 b(4.0f, -5.0f, 6.0f);
		CHECK(a.dot(b) == 12.0f);
	}

	SECTION("add and subtract")
	{
		Vector3 c = Vector3(1.0f, 1.0f, 1.0f) + Vector3(2.0f, 3.0f, 4.0f);
		CHECK(c == Vector3(3.0f, 4.0f, 5.0f));
		Vector3 d = c - Vector3(1.0f, 1.0f, 1.0f);
		CHECK(d == Vector3(2.0f, 3.0f, 4.0f));
	}
}
