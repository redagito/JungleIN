#pragma once

#include <array>

#include <glm/glm.hpp>

// View-frustum culling through oriented boxes (no GL).
// Planes use the inside-positive convention (dot(n, p) + d >= 0 keeps).
// The box test keeps straddlers, which is also what makes edge flicker
// impossible without any explicit margin: conservatism comes from the
// volume, not from a magic distance. Strictly tighter than a bounding
// sphere: a 15 m tree on a 2 m footprint gets a 15x2x2 box, not a
// 15 m balloon (which also extends 15 m underground).
struct Frustum {
  // Left, right, bottom, top, near, far.
  std::array<glm::vec4, 6> planes{};

  // Extracts the six planes from a clip matrix (proj * view). A
  // degenerate matrix yields zero planes, which keep everything
  // (fail-open: culling must never hide the scene on bad input).
  static Frustum extract(const glm::mat4& clip);

  // True when the oriented box is at least partly inside: center plus
  // orthonormal axes with per-axis half extents. Tests the positive
  // vertex (box corner farthest along the plane normal) per plane.
  bool visibleBox(const glm::vec3& center, const glm::vec3 axes[3],
    const float extents[3]) const;

 private:
  static glm::vec4 normalizePlane(const glm::vec4& plane);
};

// Model-space axis-aligned box: Mesh::boundingBox computes it once from
// the vertices, the instance transform carries it into a world-space
// oriented box per frame (exact for translate/rotate/scale compositions,
// which is all the scene graph produces).
struct ModelBox {
  glm::vec3 min = glm::vec3(0.0f);
  glm::vec3 max = glm::vec3(0.0f);
};

// Cull decision for one instance world matrix: the model box rides the
// matrix into world space (center fully transformed, axes rotated and
// normalized, extents scaled per axis). A collapsed axis contributes
// nothing, so degenerate (e.g. zero-scale) instances reduce to exact
// point tests instead of breaking. True means skip the instance.
bool CullWorldBox(const Frustum& frustum, const glm::mat4& world,
  const ModelBox& box);
