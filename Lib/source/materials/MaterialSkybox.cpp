#include "materials/MaterialSkybox.h"
#include "helpers/Logger.h"

MaterialSkyBox::MaterialSkyBox(TextureCube *textureCube)
    : Material("rt_skybox") {
  cubemap = textureCube;
  cubemap->init();
}

MaterialSkyBox::~MaterialSkyBox() {
  delete cubemap;
}

Shader *MaterialSkyBox::bind() {
  // Phase 4: drawn first, depth-tested LEQUAL so it sits behind
  // everything, but writes no depth so opaque geometry always covers it.
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LEQUAL);
  glDepthMask(GL_FALSE);
  glDisable(GL_CULL_FACE);
  glDisable(GL_BLEND);

  shader->bind();
  cubemap->bind();
  shader->transmitUniform("texCube", cubemap);

  return shader;
}
