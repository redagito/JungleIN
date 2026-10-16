#include "materials/MaterialSkybox.h"
#include "helpers/Logger.h"

MaterialSkyBox::MaterialSkyBox(std::unique_ptr<TextureCube> textureCube)
    : Material("rt_skybox"), cubemap(std::move(textureCube)) {
  cubemap->init();
}

MaterialSkyBox::~MaterialSkyBox() {
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
  shader->transmitUniform("texCube", cubemap.get());

  return shader;
}
