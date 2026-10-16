#include "helpers/Frustum.h"

#include <cmath>

Frustum Frustum::extract(const glm::mat4& clip) {
  const glm::vec4 row0(clip[0][0], clip[1][0], clip[2][0], clip[3][0]);
  const glm::vec4 row1(clip[0][1], clip[1][1], clip[2][1], clip[3][1]);
  const glm::vec4 row2(clip[0][2], clip[1][2], clip[2][2], clip[3][2]);
  const glm::vec4 row3(clip[0][3], clip[1][3], clip[2][3], clip[3][3]);
  Frustum frustum;
  frustum.planes[0] = normalizePlane(row3 + row0);
  frustum.planes[1] = normalizePlane(row3 - row0);
  frustum.planes[2] = normalizePlane(row3 + row1);
  frustum.planes[3] = normalizePlane(row3 - row1);
  frustum.planes[4] = normalizePlane(row3 + row2);
  frustum.planes[5] = normalizePlane(row3 - row2);
  return frustum;
}

bool Frustum::visibleBox(const glm::vec3& center, const glm::vec3 axes[3],
  const float extents[3]) const {
  for (const glm::vec4& plane : planes) {
    const glm::vec3 normal(plane);
    glm::vec3 positive = center;
    for (int i = 0; i < 3; ++i) {
      const float side = (axes[i].x * normal.x + axes[i].y * normal.y +
        axes[i].z * normal.z) >= 0.0f ? 1.0f : -1.0f;
      positive += axes[i] * (side * extents[i]);
    }
    const float distance = normal.x * positive.x +
      normal.y * positive.y + normal.z * positive.z + plane.w;
    if (distance < 0.0f)
      return false;
  }
  return true;
}

glm::vec4 Frustum::normalizePlane(const glm::vec4& plane) {
  const float length = std::sqrt(
    plane.x * plane.x + plane.y * plane.y + plane.z * plane.z);
  if (!(length > 0.0f))
    return glm::vec4(0.0f);
  return plane / length;
}

bool CullWorldBox(const Frustum& frustum, const glm::mat4& world,
  const ModelBox& box) {
  const glm::vec3 centerM = (box.min + box.max) * 0.5f;
  const glm::vec3 extentM = (box.max - box.min) * 0.5f;
  const glm::vec4 centerH =
    world * glm::vec4(centerM.x, centerM.y, centerM.z, 1.0f);
  const glm::vec3 center(centerH);
  glm::vec3 axes[3];
  float extents[3];
  for (int i = 0; i < 3; ++i) {
    const glm::vec3 column(world[i]);
    const float length = std::sqrt(column.x * column.x +
      column.y * column.y + column.z * column.z);
    if (length > 0.0f) {
      axes[i] = column / length;
      extents[i] = extentM[i] * length;
    }
    else {
      axes[i] = glm::vec3(1.0f, 0.0f, 0.0f);
      extents[i] = 0.0f;
    }
  }
  return !frustum.visibleBox(center, axes, extents);
}
