#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <string>
#include <vector>

#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include "render/DebugOverlay.h"
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
    window = glfwCreateWindow(800, 600, "JungleINOverlay", nullptr, nullptr);
    if (window == nullptr) {
      glfwTerminate();
      return;
    }
    glfwMakeContextCurrent(window);
    ok = gladLoadGL((GLADloadfunc)glfwGetProcAddress) != 0;
  }

  ~HiddenGL() {
    if (window != nullptr)
      glfwDestroyWindow(window);
    if (ok)
      glfwTerminate();
  }
};

} // namespace

// A flipped V axis once collapsed every glyph to a few middle rows while
// all CPU-side atlas tests stayed green (same buffer, same order). Only
// a real draw catches orientation bugs: assert ink across the full
// glyph block, not just its middle.
TEST_CASE("DebugOverlay renders full glyph rows", "[ui][overlay][gl]") {
  HiddenGL gl;
  if (!gl.ok)
    SKIP("No GL context available for overlay render test");

  DebugOverlay::setFontPath(
    (std::filesystem::path(JUNGLEIN_DATA_DIR) / "fonts" /
      "DejaVuSansMono.ttf")
      .string());

  // Only the overlay shader: absolute prefix, no repo-root CWD assumption
  // (Renderer::loadShaders uses relative data/ paths and cannot run here).
  ShaderLibrary::addShader("ss_debugtext",
    (std::filesystem::path(JUNGLEIN_DATA_DIR) / "shaders" / "ss_debugtext")
      .string(),
    {"position", "texcoord"}, {"texAtlas"});

  DebugOverlay overlay;
  overlay.setVisible(true);
  overlay.update(1.0 / 60.0);

  glViewport(0, 0, 800, 600);
  glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT);
  overlay.draw(800, 600);
  REQUIRE(glGetError() == GL_NO_ERROR);

  // First text line lives at screen y 8..32: count white pixels in its
  // upper and lower halves separately. The v-flip bug showed ink only
  // in the middle rows, starving both halves.
  const int W = 200;
  const int H = 24;
  std::vector<unsigned char> px(
    static_cast<size_t>(W) * static_cast<size_t>(H) * 4u, 0);
  glReadPixels(0, 600 - 32, W, H, GL_RGBA, GL_UNSIGNED_BYTE, px.data());

  int upper = 0;
  int lower = 0;
  for (int y = 0; y < H; ++y) {
    for (int x = 0; x < W; ++x) {
      if (px[(static_cast<size_t>(y) * W + x) * 4] > 128) {
        // GL row 0 is the screen bottom of the crop: invert for halves.
        if (y >= H / 2)
          ++upper;
        else
          ++lower;
      }
    }
  }
  CHECK(upper > 20);
  CHECK(lower > 20);
}
