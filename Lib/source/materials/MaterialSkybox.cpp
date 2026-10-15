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
  glDisable(GL_CULL_FACE);
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_BLEND);

  shader->bind();
  cubemap->bind();
  shader->transmitUniform("texCube", cubemap);

  return shader;
}
