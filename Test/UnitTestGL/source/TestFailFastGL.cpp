#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include "helpers/Geometries.h"
#include "materials/Material.h"
#include "objects/Mesh.h"
#include "render/DebugOverlay.h"
#include "render/FrameBuffer.h"
#include "render/Shader.h"
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
    window = glfwCreateWindow(1, 1, "JungleINFailFast", nullptr, nullptr);
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

// Minimal program where the registered uniform uCutoff is declared but
// never read, so the linker optimizes it out (location -1). Written to
// the temp dir so the test does not depend on the test working dir.
std::string WriteUnusedUniformShaders() {
  const std::filesystem::path dir =
      std::filesystem::temp_directory_path() / "junglein_failfast_gl";
  std::filesystem::create_directories(dir);
  const std::string prefix = (dir / "ff_unused").string();
  {
    std::ofstream vs(prefix + ".vs");
    vs << "#version 400\n"
          "layout (location = 0) in vec3 position;\n"
          "void main() { gl_Position = vec4(position, 1.0); }\n";
  }
  {
    std::ofstream fs(prefix + ".fs");
    fs << "#version 400\n"
          "uniform float uCutoff;\n"
          "layout (location = 0) out vec4 FragColor;\n"
          "void main() { FragColor = vec4(1.0); }\n";
  }
  return prefix;
}

// Exposes the protected attribute-location sentinels for the headless
// pre-init assertion. No GL calls: Geometry/Shader/Material setup here
// is pure CPU (Shader with no load carries uid -1, so its dtor is a
// no-op; Mesh with VAO 0 must throw before touching GL).
struct MeshProbe : public Mesh {
  MeshProbe(std::unique_ptr<Geometry> g, std::unique_ptr<Material> m)
    : Mesh(std::move(g), std::move(m)) {}
  int locPosition() const { return locVertices; }
  int locNormal() const { return locNormals; }
  int locTex() const { return locTexCoords; }
};

} // namespace

// Item 1: an FBO with zero targets is unusable; init must throw instead
// of logging and returning with a half-built object.
TEST_CASE("FrameBuffer init with empty targets throws",
    "[render][framebuffer][gl]")
{
  HiddenGL gl;
  if (!gl.ok)
    SKIP("No GL context available for framebuffer fail-fast test");

  FrameBuffer fb;
  CHECK_THROWS_AS(fb.init(), std::runtime_error);
}

// Item 2 (headless): attribute locations alias position's slot (0)
// before initVBO runs. The -1 sentinel must be observable pre-init.
TEST_CASE("Mesh attribute locations start unset", "[objects][mesh]")
{
  auto g = Geometries::quad(1.0);
  Shader shader("probe");
  auto m = std::make_unique<Material>(&shader);
  MeshProbe mesh(std::move(g), std::move(m));
  CHECK(mesh.locPosition() == -1);
  CHECK(mesh.locNormal() == -1);
  CHECK(mesh.locTex() == -1);
}

// Item 3: a declared-but-unused uniform optimizes out to -1. hasUniform
// must report false for it (not "registered, therefore present").
TEST_CASE("Shader hasUniform is false for optimized-out uniforms",
    "[render][shader][gl]")
{
  HiddenGL gl;
  if (!gl.ok)
    SKIP("No GL context available for shader uniform fail-fast test");

  Shader shader("ff_unused");
  shader.load(WriteUnusedUniformShaders(), {"position"}, {"uCutoff"});
  CHECK_FALSE(shader.hasUniform("uCutoff"));
  CHECK_FALSE(shader.hasUniform("undeclared-xyz"));
}

// Item 3: transmitting to an optimized-out (or unknown) uniform must
// throw instead of silently no-op-ing through glUniform(-1).
TEST_CASE("Shader transmitUniform throws for missing uniforms",
    "[render][shader][gl]")
{
  HiddenGL gl;
  if (!gl.ok)
    SKIP("No GL context available for shader uniform fail-fast test");

  Shader shader("ff_unused_tx");
  shader.load(WriteUnusedUniformShaders(), {"position"}, {"uCutoff"});
  shader.bind();
  CHECK_THROWS_AS(shader.transmitUniform("uCutoff", 0.5f),
      std::runtime_error);
  CHECK_THROWS_AS(shader.transmitUniform("undeclared-xyz", 1.0f),
      std::runtime_error);
}

// Location resolve for hot loops: same fail-fast as transmitting
// (optimized-out and undeclared both throw), but a live uniform
// resolves once to a reusable location.
TEST_CASE("Shader requireLocation resolves live uniforms",
    "[render][shader][gl]")
{
  HiddenGL gl;
  if (!gl.ok)
    SKIP("No GL context available for shader uniform fail-fast test");

  Shader dead("ff_unused_loc");
  dead.load(WriteUnusedUniformShaders(), {"position"}, {"uCutoff"});
  CHECK_THROWS_AS(dead.requireLocation("uCutoff"), std::runtime_error);
  CHECK_THROWS_AS(dead.requireLocation("undeclared-xyz"),
      std::runtime_error);

  const std::string prefix =
      (std::filesystem::path(JUNGLEIN_DATA_DIR) / "shaders" / "rt_ground_n")
          .string();
  Shader live("ff_ground_loc");
  live.load(prefix, {"position", "normal"}, {"tileSize"});
  CHECK(live.requireLocation("tileSize") >= 0);
}

// Item 4: the ground shader has no alpha-tested path, so it must not
// declare alphaCutoff. Registering the dead name proves the program
// really lacks the uniform (location -1 -> hasUniform false).
TEST_CASE("Ground program has no alphaCutoff uniform",
    "[render][ground][gl]")
{
  HiddenGL gl;
  if (!gl.ok)
    SKIP("No GL context available for ground uniform fail-fast test");

  const std::string prefix =
      (std::filesystem::path(JUNGLEIN_DATA_DIR) / "shaders" / "rt_ground_n")
          .string();
  Shader shader("ff_ground");
  shader.load(prefix, {"position", "normal"},
      {"tileSize", "alphaCutoff"});
  CHECK(shader.hasUniform("tileSize"));
  CHECK_FALSE(shader.hasUniform("alphaCutoff"));
}

// Item 5: a missing overlay shader must propagate (fail fast), not be
// swallowed. The library is a process-wide static populated by other
// tests, so: point the font at the absolute data dir (relative paths
// fail under the test CWD), clear the library, assert the throw, then
// re-register ss_debugtext from the absolute shader dir so later tests
// still find it.
TEST_CASE("DebugOverlay draw without its shader throws",
    "[ui][overlay][gl]")
{
  HiddenGL gl;
  if (!gl.ok)
    SKIP("No GL context available for overlay fail-fast test");

  DebugOverlay::setFontPath(
      (std::filesystem::path(JUNGLEIN_DATA_DIR) / "fonts" /
          "DejaVuSansMono.ttf")
          .string());
  ShaderLibrary::clear();
  DebugOverlay overlay;
  overlay.setVisible(true);
  overlay.update(1.0 / 60.0);
  CHECK_THROWS_AS(overlay.draw(800, 600), std::runtime_error);
  ShaderLibrary::addShader("ss_debugtext",
      (std::filesystem::path(JUNGLEIN_DATA_DIR) / "shaders" /
          "ss_debugtext")
          .string(),
      {"position", "texcoord"}, {"texAtlas"});
}
