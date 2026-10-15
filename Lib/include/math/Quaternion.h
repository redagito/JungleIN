#pragma once

#include "math/Vector3.h"

class Quaternion {
public:
	float x, y, z, w;

  // Default is the identity rotation, not the zero (invalid) quaternion.
  Quaternion(float x = 0.0f, float y = 0.0f, float z = 0.0f, float w = 1.0f);

	Quaternion& set(float, float, float, float);
	Quaternion& copy(const Quaternion& q);

	Quaternion& setFromVector(const Vector3& v);
	Quaternion& setFromAngle(float angle, const Vector3& axis);

	float length() const;
	Quaternion& divideScalar(float s);
	Quaternion& normalize();
};