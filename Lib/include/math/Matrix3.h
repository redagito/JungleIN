#ifndef MATRIX3_H
#define MATRIX3_H

#include "math/Matrix4.h"

class Matrix3 {
public:
  union {
    float data[3][3];
    float array[9];
  };

  Matrix3();
  Matrix3 &getInverse(const Matrix4 &m);
  Matrix3 &transpose();
  Matrix3 &identity();
};

#endif // MATRIX3_H
