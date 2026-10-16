#include "materials/MaterialWire.h"

MaterialWire::MaterialWire(const glm::vec3& color) : Material("ss_wire") {
  wireColor = color;
}

MaterialWire::~MaterialWire() {
}

Shader *MaterialWire::bind() {
  // Depth test stays on (the overlay pass clears depth first, so every
  // wire fragment passes); no depth write, no culling, no blending.
  glEnable(GL_DEPTH_TEST);
  glDepthMask(GL_FALSE);
  glDisable(GL_CULL_FACE);
  glDisable(GL_BLEND);

  shader->bind();
  shader->transmitUniform("wireColor", wireColor);

  return shader;
}
