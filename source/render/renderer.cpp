#include "renderer.h"

#include <iostream>

#include "Debug.h"

#define MAX_SHADOW_WIDTH_1 2048
#define MAX_SHADOW_WIDTH_0 2048

Renderer::Renderer() {
  active = false;
  delta = 0;

  width = 1024;
  height = 768;
  finalWidth = width;
  finalHeight = height;
  ratio = 1.0;

  mainFBO = 0;
  shadowFBO = 0;
  shadowMat = 0;
  postComposer = 0;

  __camera = 0;
  __material = 0;
  __shader = 0;
  __locPosition = 0;
  __locColor = 0;
  __locNormal = 0;
  __locTexcoord = 0;
  __locMVP = 0;

  useShadowCascade = true;
}

void Renderer::init(Scene *_scene, unsigned int w, unsigned int h) {
  shadowMat = new MaterialShadow();
  PRINT_GL_ERROR();

  width = w;
  height = h;

  mainFBO = new FrameBuffer(std::vector<std::string>() << "texScene"
                                                       << "texThreshold",
                            w, h, true);
  PRINT_GL_ERROR();
  mainFBO->init(0, true);
  PRINT_GL_ERROR();

  shadowFBO =
      new ShadowBuffer("texShadow", MAX_SHADOW_WIDTH_1, MAX_SHADOW_WIDTH_1);
  PRINT_GL_ERROR();
  shadowFBO->init(7);
  PRINT_GL_ERROR();

  shadowFBOCascade = new ShadowBuffer("texShadowCascade", MAX_SHADOW_WIDTH_1,
                                      MAX_SHADOW_WIDTH_1);
  PRINT_GL_ERROR();
  shadowFBOCascade->init(8);
  PRINT_GL_ERROR();

  postComposer = new PostComposer(mainFBO);
  PRINT_GL_ERROR();

  scene = _scene;
  __camera = scene->getCurrentCamera();
  __skybox = scene->getSky();
  __sun = scene->getSun();
  __ground = scene->getGround();

  matrixProj = __camera->getProjection();
  matrixView = __camera->getMatrix();
  matrixModel.identity();
  xform.push(matrixModel);
  PRINT_GL_ERROR();
}

void Renderer::start() {
  active = true;
  timer.start();
}

void Renderer::stop() { active = false; }

void Renderer::render() {
	PRINT_GL_ERROR();
  // t.start();

  if (!active || !scene->getRenderAble())
    return;

  delta = timer.restart() / Utils::TARGET_FPS;

  scene->update(delta);
  PRINT_GL_ERROR();
  __lightDir = __sun->getLight()->getDirection();

  glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
  PRINT_GL_ERROR();
  glClearDepth(1.0f);
  PRINT_GL_ERROR();

  renderShadow();
  PRINT_GL_ERROR();

  mainFBO->bind();
  PRINT_GL_ERROR();
  mainFBO->resizeViewport();
  PRINT_GL_ERROR();
  mainFBO->clear();
  PRINT_GL_ERROR();

  matrixProj = __camera->getProjection();
  matrixView = __camera->getMatrix();

  drawSky();
  PRINT_GL_ERROR();
  drawSun();
  PRINT_GL_ERROR();

  // matrixProj = __sun->getLight()->getProjMatrix();
  // matrixView = __sun->getLight()->getViewMatrix();

  pushMatrix(scene->getMatrix());

  drawGround();
  PRINT_GL_ERROR();

  drawMeshes();
  PRINT_GL_ERROR();

  popMatrix();

  mainFBO->unbind();
  PRINT_GL_ERROR();

  postComposer->render();
  PRINT_GL_ERROR();

  Texture::needsUpdate = false;

  Utils::calculFPS();
  PRINT_GL_ERROR();
  
  // postComposer->debugQuad(shadowFBOCascade->getTexture("texShadowCascade"));
}

void Renderer::renderShadow() {
  shadowFBO->bind();
  shadowFBO->resizeViewport();
  shadowFBO->clear();

  matrixProj = __sun->getLight()->getProjMatrix();
  matrixView = __sun->getLight()->getViewMatrix();

  pushMatrix(scene->getMatrix());

  drawMeshesShadow();

  popMatrix();

  shadowFBO->unbind();

  if (useShadowCascade) {
    shadowFBOCascade->bind();
    shadowFBOCascade->resizeViewport();
    shadowFBOCascade->clear();

    matrixProj = __sun->getLight()->getProjMatrixCascade();
    matrixView = __sun->getLight()->getViewMatrix();

    pushMatrix(scene->getMatrix());

    drawMeshesShadow();

    popMatrix();

    shadowFBOCascade->unbind();
  }
}

void Renderer::drawSky() {
  pushMatrix(__skybox->getMatrix());

  __material = __skybox->getMaterial();
  __shader = __material->bind();

  calcMVP();
  transmitMVP();

  __skybox->drawWithVBO();

  popMatrix();
}

void Renderer::drawSun() {
  pushMatrix(__sun->getMatrix());

  __material = __sun->getMaterial();
  __shader = __material->bind();

  calcMVP();
  transmitMVP();

  postComposer->setLightPosScreen(
      Vector3(__mvp.data[0][3], __mvp.data[1][3], __mvp.data[2][3])
              .divideScalar(__mvp.data[3][3]) *
          0.5 +
      Vector3(0.5, 0.5, 0.5));

  __sun->drawWithVBO();

  popMatrix();
}

void Renderer::drawGround() {
  pushMatrix(__ground->getMatrix());

  __material = __ground->getMaterial();
  __shader = __material->bind();

  shadowFBO->bindAndTransmitTextures(__shader);
  if (useShadowCascade) {
    shadowFBOCascade->bindAndTransmitTextures(__shader);
    __shader->transmitUniform("cascadeShadow", 1);
  } else {
    __shader->transmitUniform("cascadeShadow", 0);
  }

  calcMVP();
  transmitNormalMatrix();
  transmitViewMatrix();
  transmitMVP();
  transmitShadowMatrix();
  __shader->transmitUniform("lightDir", __lightDir);
  __shader->transmitUniform("shadowResolution", (float)shadowFBO->getWidth());
  __shader->transmitUniform("shadowQuality", Utils::QUALITY);

  __ground->drawWithVBO();

  popMatrix();
}

bool Renderer::cull(Instance *m, float offset) {
  return (m->getPosition().distanceSq(__camera->getPosition()) > 500 + offset &&
          (m->getPosition() -
           (__camera->getPosition() - scene->getCameraZ() * 20.0))
                  .dot(scene->getCameraZ()) < 0);
}

void Renderer::drawMeshes() {
  for (Mesh *m : scene->getMeshes()) {
    __material = m->getMaterial();
    __shader = __material->bind();

    shadowFBO->bindAndTransmitTextures(__shader);
    if (useShadowCascade) {
      shadowFBOCascade->bindAndTransmitTextures(__shader);
      __shader->transmitUniform("cascadeShadow", 1);
    } else {
      __shader->transmitUniform("cascadeShadow", 0);
    }

    __shader->transmitUniform("lightDir", __lightDir);
    __shader->transmitUniform("shadowResolution", (float)shadowFBO->getWidth());
    __shader->transmitUniform("shadowQuality", Utils::QUALITY);

    pushMatrix(m->getMatrix());
    calcMVP();
    transmitNormalMatrix();
    transmitViewMatrix();
    transmitMVP();
    transmitShadowMatrix();

    m->drawWithVBO();

    popMatrix();

    for (Instance *i : m->getInstances()) {
      if (cull(i, 250)) {
        continue;
      }

      pushMatrix(i->getMatrix());
      calcMVP();
      transmitNormalMatrix();
      transmitViewMatrix();
      transmitMVP();
      transmitShadowMatrix();

      m->drawWithVBO();

      popMatrix();
    }
  }
}

void Renderer::drawMeshesShadow() {
  for (Mesh *m : scene->getMeshes()) {
    if (!m->castsShadows())
      continue;

    __material = shadowMat;
    __shader = __material->bind();

    if (((MaterialBasic *)m->getMaterial())->hasAlpha()) {
      Texture *alpha = ((MaterialBasic *)m->getMaterial())->getAlpha();
      alpha->bind();
      __shader->transmitUniform("texAlpha", alpha);
      __shader->transmitUniform("useAlpha", 1);
    } else {
      __shader->transmitUniform("useAlpha", 0);
    }

    pushMatrix(m->getMatrix());
    calcMVP();
    transmitMVP();

    m->drawWithVBO();

    popMatrix();

    for (Instance *i : m->getInstances()) {
      if (cull(i, 2000))
        continue;

      pushMatrix(i->getMatrix());
      calcMVP();
      transmitMVP();

      m->drawWithVBO();

      popMatrix();
    }
  }
}

void Renderer::resize(unsigned int w, unsigned int h) {
  finalWidth = w;
  finalHeight = h;
  float a = w / static_cast<float>(h);

  if (ratio != 1.0) {
    w = (int)std::floor(w * ratio + .5);
    h = (int)std::floor(w / a + .5);
  }

  if (postComposer != 0) {
    postComposer->resize(w, h);
    postComposer->setFinalRenderSize(finalWidth, finalHeight);
  }

  if (mainFBO != 0)
    mainFBO->resize(w, h);
  if (shadowFBO != 0) {
    int ws = w * 2.0;
    int max = Utils::QUALITY == 1 ? MAX_SHADOW_WIDTH_1 : MAX_SHADOW_WIDTH_0;
    if (ws > max)
      ws = max;
    shadowFBO->resize(ws, ws);
  }
  if (shadowFBOCascade != 0) {
    int ws = w * 2.0;
    int max = Utils::QUALITY == 1 ? MAX_SHADOW_WIDTH_1 : MAX_SHADOW_WIDTH_0;
    if (ws > max)
      ws = max;
    shadowFBOCascade->resize(ws, ws);
  }
  if (__sun != 0)
    __sun->getLight()->resize(1);

  if (__camera != 0) {
    __camera->setPerspective(65.0f, a, 0.1f, 500.0f);
    matrixProj = __camera->getProjection();
  }
}

PostComposer *Renderer::getPostComposer() { return postComposer; }

void Renderer::pushMatrix(const Matrix4 &mat) {
  xform.push(matrixModel);
  matrixModel *= mat;
}

void Renderer::popMatrix() {
  matrixModel = xform.top();
  xform.pop();
}

void Renderer::calcMVP() {
  __mv = matrixView * matrixModel;
  __mvp = matrixProj * __mv;
}

void Renderer::transmitShadowMatrix() {
  __shader->transmitUniform(
      "shadowMatrix", __sun->getLight()->getTextureProjMatrix(matrixModel));
  if (useShadowCascade)
    __shader->transmitUniform(
        "shadowMatrixCascade",
        __sun->getLight()->getTextureProjMatrixCascade(matrixModel));
}

void Renderer::transmitMVP() { __shader->transmitUniform("mvp", __mvp); }

void Renderer::transmitNormalMatrix() {
  __shader->transmitUniform("normalMatrix",
                            Matrix3().getInverse(__mv).transpose());
}

void Renderer::transmitViewMatrix() {
  __shader->transmitUniform("viewMatrix",
                            Matrix3().getInverse(matrixView).transpose());
}

void Renderer::toggleShadowCascade(bool active) {
  useShadowCascade = active;
  __sun->getLight()->setSize(active ? 20.0 : 120.0);
  __sun->getLight()->resize(1.0);
}

void Renderer::setRatio(float r) {
  ratio = r > 0.0 && r < 1.0 ? r : 1.0;
  resize(finalWidth, finalHeight);
}

void Renderer::loadShaders() {
  ShaderLibrary::addShader("rt_basic", "data/shaders/rt_basic",
                           std::vector<std::string>() << "position"
                                                      << "normal"
                                                      << "texcoord",
                           std::vector<std::string>() << "mvp"
                                                      << "normalMatrix"
                                                      << "viewMatrix"
                                                      << "lightDir"
                                                      << "texDiffuse"
                                                      << "texAlpha"
                                                      << "shadowQuality"
                                                      << "texShadow"
                                                      << "shadowMatrix"
                                                      << "shadowResolution"
                                                      << "texShadowCascade"
                                                      << "shadowMatrixCascade"
                                                      << "cascadeShadow");

  ShaderLibrary::addShader("rt_sun", "data/shaders/rt_sun",
                           std::vector<std::string>() << "position"
                                                      << "texcoord",
                           std::vector<std::string>() << "mvp"
                                                      << "texDiffuse"
                                                      << "texAlpha");

  ShaderLibrary::addShader("rt_ground", "data/shaders/rt_ground",
                           std::vector<std::string>() << "position"
                                                      << "normal",
                           std::vector<std::string>() << "mvp"
                                                      << "normalMatrix"
                                                      << "viewMatrix"
                                                      << "lightDir"
                                                      << "heightRange"
                                                      << "tileSize"
                                                      << "texMoss"
                                                      << "texEarth"
                                                      << "texShatter"
                                                      << "texNormalMoss"
                                                      << "texNormalEarth"
                                                      << "texNormalShatter"
                                                      << "shadowQuality"
                                                      << "texShadow"
                                                      << "shadowMatrix"
                                                      << "shadowResolution"
                                                      << "texShadowCascade"
                                                      << "shadowMatrixCascade"
                                                      << "cascadeShadow");

  ShaderLibrary::addShader("rt_shadow", "data/shaders/rt_shadow",
                           std::vector<std::string>() << "position"
                                                      << "texcoord",
                           std::vector<std::string>() << "mvp"
                                                      << "texAlpha"
                                                      << "useAlpha");

  ShaderLibrary::addShader("rt_skybox", "data/shaders/rt_skybox",
                           std::vector<std::string>() << "position",
                           std::vector<std::string>() << "mvp"
                                                      << "texCube");

  ShaderLibrary::addShader("ss_compose", "data/shaders/ss_compose",
                           std::vector<std::string>() << "position",
                           std::vector<std::string>() << "texScene");

  ShaderLibrary::addShader("ss_display", "data/shaders/ss_display",
                           std::vector<std::string>() << "position",
                           std::vector<std::string>() << "texScene"
                                                      << "gammaRGB"
                                                      << "aspect");

  ShaderLibrary::addShader("ss_fxaa", "data/shaders/ss_fxaa",
                           std::vector<std::string>() << "position",
                           std::vector<std::string>() << "texScene"
                                                      << "resolution");

  ShaderLibrary::addShader("ss_lenseflare_p1", "data/shaders/ss_lenseflare_p1",
                           std::vector<std::string>() << "position",
                           std::vector<std::string>() << "texScene"
                                                      << "flareThreshold");

  ShaderLibrary::addShader("ss_lenseflare_p2", "data/shaders/ss_lenseflare_p2",
                           std::vector<std::string>() << "position",
                           std::vector<std::string>()
                               << "texScene"
                               << "flareSamples"
                               << "flareDispersal"
                               << "flareHaloWidth"
                               << "flareChromaDispertion");

  ShaderLibrary::addShader("ss_lenseflare_p3", "data/shaders/ss_lenseflare_p3",
                           std::vector<std::string>() << "position",
                           std::vector<std::string>() << "texScene"
                                                      << "texFlare"
                                                      << "texDirt"
                                                      << "flareIntensity");

  ShaderLibrary::addShader("ss_blur_linearh", "data/shaders/ss_blur_linearh",
                           std::vector<std::string>() << "position",
                           std::vector<std::string>() << "texScene"
                                                      << "resolution");

  ShaderLibrary::addShader("ss_blur_linearv", "data/shaders/ss_blur_linearv",
                           std::vector<std::string>() << "position",
                           std::vector<std::string>() << "texScene"
                                                      << "resolution");

  ShaderLibrary::addShader("ss_distortion", "data/shaders/ss_distortion",
                           std::vector<std::string>() << "position",
                           std::vector<std::string>() << "texScene"
                                                      << "resolution");

  ShaderLibrary::addShader("ss_godrays_p1", "data/shaders/ss_godrays_p1",
                           std::vector<std::string>() << "position",
                           std::vector<std::string>() << "texScene"
                                                      << "lightPos");

  ShaderLibrary::addShader("ss_godrays_p2", "data/shaders/ss_godrays_p2",
                           std::vector<std::string>() << "position",
                           std::vector<std::string>() << "texScene"
                                                      << "texGodrays");

  ShaderLibrary::addShader("ss_bloom_p1", "data/shaders/ss_bloom_p1",
                           std::vector<std::string>() << "position",
                           std::vector<std::string>() << "texScene");

  ShaderLibrary::addShader("ss_bloom_p2", "data/shaders/ss_bloom_p2",
                           std::vector<std::string>() << "position",
                           std::vector<std::string>() << "texScene"
                                                      << "texBloom");
}
