#pragma once

#include <glm/glm.hpp>

#include "materials/Material.h"

// Flat unlit wireframe material for the diagnostic overlay pass (key 0).
// Solid wireColor so lines contrast against the shaded fill underneath:
// rebinding the lit materials here would shade lines identically to the
// surface and render the overlay invisible.
class MaterialWire : public Material {
public:
  explicit MaterialWire(
    const glm::vec3& color = glm::vec3(0.0f));
  ~MaterialWire() override;
  Shader *bind();

private:
  glm::vec3 wireColor;
};
