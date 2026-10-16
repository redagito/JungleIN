// Explicit texture-unit assignment: units are chosen by the caller per
// purpose (see the unit map on Texture), never handed out by a shared
// counter. These tests pin the map and the two-materials-in-sequence
// sampling behavior inside a hidden GL context (same HiddenGL pattern
// as TestShaderCompile).
#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_test_macros.hpp>

#include <memory>
#include <string>
#include <vector>

#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include "core/Texture.h"
#include "materials/MaterialBasic.h"
#include "render/ShaderLibrary.h"

namespace {

struct HiddenGL {
  GLFWwindow *window = nullptr;
  bool ok = false;

  HiddenGL() {
    if (glfwInit() == 0)
      return;
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    window = glfwCreateWindow(1, 1, "JungleINTexUnits", nullptr, nullptr);
    if (window == nullptr) {
      glfwTerminate();
      return;
    }
    glfwMakeContextCurrent(window);
    ok = gladLoadGL((GLADloadfunc)glfwGetProcAddress) != 0;
    // Fresh library per context: static Shader objects outlive their
    // per-test GL context, so a later case would reuse programs compiled
    // for a dead context (silent empty draws). See resetActiveShader.
    if (ok)
      ShaderLibrary::clear();
  }

  ~HiddenGL() {
    if (window != nullptr)
      glfwDestroyWindow(window);
    if (ok)
      glfwTerminate();
  }
};

// Minimal rt_basic registration with the exact attribute/uniform lists
// from Renderer::loadShaders (duplicated so the fixture stands alone).
void RegisterBasicShader() {
  const std::string dir = std::string(JUNGLEIN_DATA_DIR) + "/shaders/";
  ShaderLibrary::addShader(
      "rt_basic", dir + "rt_basic", {"position", "normal", "texcoord"},
      {"mvp", "mv", "normalMatrix", "viewMatrix", "lightDir",
       "lightIntensity", "texDiffuse", "texAlpha", "alphaCutoff",
       "useDiffuse", "useAlpha", "diffuseColor", "shadowQuality",
       "texShadow", "shadowMatrix", "texShadowCascade",
       "shadowMatrixCascade", "cascadeShadow"});
}

} // namespace

// The constructor index IS the unit: creation order must not matter
// (the old global counter handed out whatever came next).
TEST_CASE("Texture units are explicit per purpose", "[texunits]")
{
  HiddenGL gl;
  if (!gl.ok)
    SKIP("No GL context available for texture unit test");

  Texture second(1u);
  Texture first(0u);
  CHECK(first.unit() == 0u);
  CHECK(second.unit() == 1u);
  CHECK(first.glunit == GL_TEXTURE0 + 0u);
  CHECK(second.glunit == GL_TEXTURE0 + 1u);

  first.setUnit(5u);
  CHECK(first.unit() == 5u);
  CHECK(first.glunit == GL_TEXTURE0 + 5u);
  // Reassignment does not move anyone else.
  CHECK(second.unit() == 1u);
}

// The documented unit map (see Texture.h): every texture that can be
// sampled in one draw owns a distinct unit. Same units reused across
// draws are fine because each bind() rebinds before transmitting.
TEST_CASE("Scene unit map stays alias-free within a draw", "[texunits]")
{
  HiddenGL gl;
  if (!gl.ok)
    SKIP("No GL context available for texture unit test");

  // Ground set 0..5 plus the two shadow maps 7..8, as wired by
  // SceneBuilder and Renderer.
  std::vector<unsigned int> coLive = {0, 1, 2, 3, 4, 5, 7, 8};
  std::vector<Texture *> textures;
  for (unsigned int unit : coLive)
    textures.push_back(new Texture(unit));
  for (size_t i = 0; i < textures.size(); ++i) {
    INFO("texture index " << i);
    CHECK(textures[i]->unit() == coLive[i]);
    for (size_t j = 0; j < textures.size(); ++j) {
      if (i == j)
        continue;
      CHECK(textures[i]->unit() != textures[j]->unit());
    }
  }
  for (Texture *tex : textures)
    delete tex;
}

// Two MaterialBasic draws back to back over the same shader: the
// second bind must re-establish units 0/1 (no leak from the first
// material, no alias exception), so each draw samples its own maps.
TEST_CASE("Two materials in sequence sample their own units", "[texunits][gl]")
{
  HiddenGL gl;
  if (!gl.ok)
    SKIP("No GL context available for texture unit test");
  RegisterBasicShader();

  auto diffuseA = std::make_unique<Texture>(0u);
  auto alphaA = std::make_unique<Texture>(1u);
  auto diffuseB = std::make_unique<Texture>(0u);
  auto alphaB = std::make_unique<Texture>(1u);
  // Borrowed views: ownership moves into the materials below, but the
  // objects outlive this scope, so raw observation stays valid.
  Texture *diffuseAView = diffuseA.get();
  Texture *alphaAView = alphaA.get();
  Texture *diffuseBView = diffuseB.get();
  Texture *alphaBView = alphaB.get();

  MaterialBasic matA(glm::vec3(1.0f));
  matA.setDiffuse(std::move(diffuseA));
  matA.setAlpha(std::move(alphaA));
  MaterialBasic matB(glm::vec3(0.5f));
  matB.setDiffuse(std::move(diffuseB));
  matB.setAlpha(std::move(alphaB));

  REQUIRE_NOTHROW(matA.bind());
  CHECK(diffuseAView->unit() == 0u);
  CHECK(alphaAView->unit() == 1u);
  REQUIRE_NOTHROW(matB.bind());
  CHECK(diffuseBView->unit() == 0u);
  CHECK(alphaBView->unit() == 1u);
  // First material's units are untouched by the second bind.
  CHECK(diffuseAView->unit() == 0u);
  CHECK(alphaAView->unit() == 1u);

  ShaderLibrary::clear();
}

// Wrap mode is per-texture state (default REPEAT); the ground maps use
// MIRRORED_REPEAT so non-seamless assets don't filtering-seam at tile
// edges. Needs a GL context only because ~Texture calls glDeleteTextures.
TEST_CASE("Texture wrap mode defaults and round-trips", "[core][texture]") {
  HiddenGL gl;
  if (!gl.ok)
    SKIP("No GL context available for texture wrap test");
  Texture tex(0u);
  CHECK(tex.wrapS() == GL_REPEAT);
  CHECK(tex.wrapT() == GL_REPEAT);
  tex.setWrap(GL_MIRRORED_REPEAT, GL_CLAMP_TO_EDGE);
  CHECK(tex.wrapS() == GL_MIRRORED_REPEAT);
  CHECK(tex.wrapT() == GL_CLAMP_TO_EDGE);
}
