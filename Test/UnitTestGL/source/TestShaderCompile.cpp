#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <string>
#include <vector>

#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include "render/Shader.h"

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
    window = glfwCreateWindow(1, 1, "JungleINTests", nullptr, nullptr);
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

// Compiles and links every shader pair found in the data dir inside a
// hidden GL context. Fails with the driver log on the first broken
// shader. Driver remarks (non-empty info logs on success) are reported
// through the logger; run this binary directly to read them.
TEST_CASE("Shaders compile and link", "[render][shaders][gl]")
{
  HiddenGL gl;
  if (!gl.ok)
    SKIP("No GL context available for shader compilation test");

  const std::filesystem::path shaderDir =
      std::filesystem::path(JUNGLEIN_DATA_DIR) / "shaders";
  REQUIRE(std::filesystem::is_directory(shaderDir));

  size_t checked = 0;
  for (const auto &entry : std::filesystem::directory_iterator(shaderDir)) {
    if (entry.path().extension() != ".vs")
      continue;
    const std::string prefix =
        (shaderDir / entry.path().stem()).string();
    INFO("shader: " << entry.path().stem().string());
    if (!std::filesystem::exists(prefix + ".fs")) {
      FAIL("Vertex shader without fragment pair: " << prefix);
    }
    Shader shader(entry.path().stem().string());
    shader.load(prefix, {}, {});
    ++checked;
  }
  CHECK(checked > 0);
}
