#include "math/Quaternion.h"

#include <cmath>
#include <limits>

#include "math/Math.h"

Quaternion::Quaternion(float x_, float y_, float z_, float w_)
    : x(x_), y(y_), z(z_), w(w_) {}

Quaternion &Quaternion::set(float x_, float y_, float z_, float w_) {
  this->x = x_;
  this->y = y_;
  this->z = z_;
  this->w = w_;

  return *this;
}

Quaternion &Quaternion::copy(const Quaternion &v) {
  x = v.x;
  y = v.y;
  z = v.z;
  w = v.w;

  return *this;
}

/*!
 * @see
 * http://en.wikipedia.org/wiki/Conversion_between_quaternions_and_Euler_angles
 */
Quaternion &Quaternion::setFromVector(const Vector3 &v) {
  float c = static_cast<float>(Pi / 360), ex = v.x * c, ey = v.y * c,
        ez = v.z * c,

        c1 = std::cos(ey), s1 = std::sin(ey), c2 = std::cos(-ez),
        s2 = std::sin(-ez), c3 = std::cos(ex), s3 = std::sin(ex),

        c1c2 = c1 * c2, s1s2 = s1 * s2;

  this->w = c1c2 * c3 - s1s2 * s3;
  this->x = c1c2 * s3 + s1s2 * c3;
  this->y = s1 * c2 * c3 + c1 * s2 * s3;
  this->z = c1 * s2 * c3 - s1 * c2 * s3;

  return *this;
}

Quaternion &Quaternion::setFromAngle(float angle, const Vector3 &axis) {
  float halfAngle = angle / 2, s = std::sin(halfAngle);

  this->x = axis.x * s;
  this->y = axis.y * s;
  this->z = axis.z * s;
  this->w = std::cos(halfAngle);

  return *this;
}

float Quaternion::length() const { return std::sqrt(x * x + y * y + z * z + w * w); }

Quaternion &Quaternion::divideScalar(float s) {
  if (s > std::numeric_limits<float>::epsilon()) {
    x /= s;
    y /= s;
    z /= s;
    w /= s;
  } else {
    x = 0;
    y = 0;
    z = 0;
    w = 0;
  }

  return *this;
}

Quaternion &Quaternion::normalize() { return divideScalar(length()); }
