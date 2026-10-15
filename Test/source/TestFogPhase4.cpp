#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

namespace {

std::string ReadFile(const std::filesystem::path &p) {
  std::ifstream in(p);
  REQUIRE(in.good());
  std::ostringstream body;
  body << in.rdbuf();
  return body.str();
}

// First line containing the key, trimmed of leading whitespace.
std::string FindLine(const std::string &body, const std::string &key) {
  std::istringstream lines(body);
  std::string line;
  while (std::getline(lines, line)) {
    if (line.find(key) != std::string::npos) {
      const std::string::size_type first = line.find_first_not_of(" \t\r");
      return first == std::string::npos ? line : line.substr(first);
    }
  }
  return std::string();
}

std::filesystem::path ShaderDir() {
  return std::filesystem::path(JUNGLEIN_DATA_DIR) / "shaders";
}

std::filesystem::path RepoRoot() {
  return std::filesystem::path(JUNGLEIN_DATA_DIR).parent_path();
}

} // namespace

// Phase 4 united the fog model: both lit shaders share one FOG_COLOR and
// one FOG_DENSITY (ground_n values adopted), read vViewDepth, and never
// touch gl_FragCoord for fog.
TEST_CASE("Phase4 lit shaders share one fog model", "[render][fog]")
{
  const std::filesystem::path dir = ShaderDir();
  REQUIRE(std::filesystem::is_directory(dir));

  const std::string basic = ReadFile(dir / "rt_basic.fs");
  const std::string ground = ReadFile(dir / "rt_ground_n.fs");

  const std::string basicColor = FindLine(basic, "FOG_COLOR =");
  const std::string groundColor = FindLine(ground, "FOG_COLOR =");
  const std::string basicDensity = FindLine(basic, "FOG_DENSITY =");
  const std::string groundDensity = FindLine(ground, "FOG_DENSITY =");

  REQUIRE_FALSE(basicColor.empty());
  REQUIRE_FALSE(groundColor.empty());
  REQUIRE_FALSE(basicDensity.empty());
  REQUIRE_FALSE(groundDensity.empty());

  // Same constants in both shaders (the unified model).
  CHECK(basicColor == groundColor);
  CHECK(basicDensity == groundDensity);
  // Ground_n values adopted: document the pick.
  CHECK(basicColor.find("0.796") != std::string::npos);
  CHECK(basicColor.find("0.949") != std::string::npos);
  CHECK(basicDensity.find("0.006") != std::string::npos);

  for (const std::string *fs : {&basic, &ground}) {
    INFO("fog body check");
    // View-depth varying, never window coordinates.
    CHECK(fs->find("vViewDepth") != std::string::npos);
    CHECK(fs->find("gl_FragCoord") == std::string::npos);
    // Phase 2 shadow contract untouched.
    CHECK(fs->find("sampler2DShadow") != std::string::npos);
    CHECK(fs->find("textureProj(") != std::string::npos);
  }
}

// Sky blends to the shared fog color at the horizon; the sun documents
// why it stays unfogged. Neither may use gl_FragCoord for fog.
TEST_CASE("Phase4 sky and sun fog contract", "[render][fog]")
{
  const std::filesystem::path dir = ShaderDir();
  const std::string sky = ReadFile(dir / "rt_skybox.fs");
  const std::string sun = ReadFile(dir / "rt_sun.fs");

  CHECK(sky.find("gl_FragCoord") == std::string::npos);
  CHECK(sun.find("gl_FragCoord") == std::string::npos);
  // Sky reuses the unified fog color at the horizon.
  CHECK(sky.find("FOG_COLOR") != std::string::npos);
  CHECK(sky.find("0.796") != std::string::npos);
  // Sun is explicitly unfogged with a reason, not silently skipped.
  CHECK(sun.find("unfogged") != std::string::npos);
}

// FOV mismatch removed: constructor and Renderer::resize share one 65
// degree default, near 0.1 / far 500 kept (skybox recenters every frame,
// so no 800 far plane is needed).
TEST_CASE("Phase4 camera FOV is unified", "[render][camera]")
{
  const std::filesystem::path root = RepoRoot();
  const std::string cameraCpp =
      ReadFile(root / "Lib" / "source" / "objects" / "Camera.cpp");
  const std::string cameraH =
      ReadFile(root / "Lib" / "include" / "objects" / "Camera.h");
  const std::string rendererCpp =
      ReadFile(root / "Lib" / "source" / "render" / "Renderer.cpp");

  CHECK(cameraH.find("kDefaultFov") != std::string::npos);
  CHECK(cameraH.find("65.0f") != std::string::npos);
  CHECK(cameraH.find("500.0f") != std::string::npos);
  CHECK(cameraCpp.find("90.0f") == std::string::npos);
  CHECK(cameraCpp.find("kDefaultFov") != std::string::npos);
  CHECK(rendererCpp.find("90.0f") == std::string::npos);
  CHECK(rendererCpp.find("kDefaultFov") != std::string::npos);
}

// Skybox follows the camera every frame with LEQUAL + no depth write,
// drawn first; depth-write restore lives in the opaque materials.
TEST_CASE("Phase4 sky follows camera with sky depth state", "[render][sky]")
{
  const std::filesystem::path root = RepoRoot();
  const std::string sceneCpp =
      ReadFile(root / "Lib" / "source" / "objects" / "Scene.cpp");
  const std::string rendererCpp =
      ReadFile(root / "Lib" / "source" / "render" / "Renderer.cpp");
  const std::string skyMat =
      ReadFile(root / "Lib" / "source" / "materials" / "MaterialSkybox.cpp");
  const std::string basicMat =
      ReadFile(root / "Lib" / "source" / "materials" / "MaterialBasic.cpp");
  const std::string groundMat =
      ReadFile(root / "Lib" / "source" / "materials" / "MaterialGround.cpp");

  // Per-frame follow, not just spawn.
  CHECK(sceneCpp.find("sky->setPosition") != std::string::npos);
  CHECK(rendererCpp.find("setPosition(__camera") != std::string::npos);
  // Sun anchor projection kept for god rays.
  CHECK(rendererCpp.find("clipW") != std::string::npos);
  // Sky depth state: tested, LEQUAL, writes nothing.
  CHECK(skyMat.find("GL_LEQUAL") != std::string::npos);
  CHECK(skyMat.find("glDepthMask(GL_FALSE)") != std::string::npos);
  CHECK(skyMat.find("glDisable(GL_DEPTH_TEST)") == std::string::npos);
  // Opaque restores depth write in the material, not the renderer.
  CHECK(basicMat.find("glDepthMask(GL_TRUE)") != std::string::npos);
  CHECK(groundMat.find("glDepthMask(GL_TRUE)") != std::string::npos);
  CHECK(rendererCpp.find("glDepthMask(GL_TRUE)") == std::string::npos);
}

// Every post pass targets an explicit viewport sized from its own
// buffers; gamma 2.2 applies once in the final display pass.
TEST_CASE("Phase4 post viewports and single gamma", "[render][post]")
{
  const std::filesystem::path root = RepoRoot();
  const std::string composer =
      ReadFile(root / "Lib" / "source" / "render" / "PostComposer.cpp");
  const std::string display =
      ReadFile(ShaderDir() / "ss_display.fs");

  // Explicit viewport per pass from the target buffer size.
  CHECK(composer.find("readBuffer->resizeViewport") != std::string::npos);
  CHECK(composer.find("writeBuffer->resizeViewport") != std::string::npos);
  CHECK(composer.find("outBuffer->resizeViewport") != std::string::npos);
  // Single gamma in display: 2.2 default, no 1.0 identity left.
  CHECK(composer.find("2.2f") != std::string::npos);
  CHECK(composer.find("Vector3(1.0, 1.0, 1.0)") == std::string::npos);
  CHECK(display.find("GammaCorrection") != std::string::npos);
  // Effect passes untouched.
  CHECK(composer.find("passBloom") != std::string::npos);
  CHECK(composer.find("passGodrays") != std::string::npos);
  CHECK(composer.find("passLenseFlare") != std::string::npos);
}
