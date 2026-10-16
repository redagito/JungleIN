#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <string>
#include <vector>

#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include "render/Renderer.h"
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
    window = glfwCreateWindow(1, 1, "JungleINUniforms", nullptr, nullptr);
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

// Renderer::loadShaders resolves data/ relatively, so run it from the
// repo root and restore the working directory on the way out.
struct RepoRootCwd {
  std::filesystem::path previous;
  RepoRootCwd() : previous(std::filesystem::current_path()) {
    std::filesystem::current_path(
      std::filesystem::path(JUNGLEIN_DATA_DIR).parent_path());
  }
  ~RepoRootCwd() {
    std::error_code code;
    std::filesystem::current_path(previous, code);
  }
};

} // namespace

// A registered-but-unused uniform links to location -1, and the fail-fast
// transmit throws on it at draw time (once a startup crash). Every
// registration must resolve, for every forward shader.
TEST_CASE("All registered uniforms resolve to locations",
  "[render][shaders][gl]") {
  HiddenGL gl;
  if (!gl.ok)
    SKIP("No GL context available for uniform registration test");

  RepoRootCwd root;
  Renderer renderer;
  renderer.loadShaders();

  static const char *const kShaders[] = {
    "rt_basic", "rt_ground_n", "rt_shadow", "rt_skybox", "rt_sun",
    "ss_debugtext", "ss_wire"};
  for (const char *name : kShaders) {
    Shader *shader = ShaderLibrary::getShader(name);
    for (const std::string &uniform : shader->uniformNames())
      CHECK(shader->hasUniform(uniform));
  }
}
