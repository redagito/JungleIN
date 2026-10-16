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

std::filesystem::path ShaderDir() {
  return std::filesystem::path(JUNGLEIN_DATA_DIR) / "shaders";
}

} // namespace

// sRGB inputs must be linearized before lighting; the display pass
// gamma-encodes once. Without the decode, mid-tones render ~1.5x too
// bright. Normal maps and alpha masks are data and stay raw.
TEST_CASE("Color pipeline decodes sRGB inputs to linear", "[render][color]")
{
  const std::filesystem::path dir = ShaderDir();
  REQUIRE(std::filesystem::is_directory(dir));

  const std::string basic = ReadFile(dir / "rt_basic.fs");
  const std::string ground = ReadFile(dir / "rt_ground_n.fs");
  const std::string sky = ReadFile(dir / "rt_skybox.fs");
  const std::string sun = ReadFile(dir / "rt_sun.fs");

  for (const std::string *fs : {&basic, &ground}) {
    INFO("lit shader decode check");
    // Exact 2.2 decode matching the display 2.2 encode.
    CHECK(fs->find("vec3(2.2)") != std::string::npos);
  }
  // Albedo decoded in both lit shaders.
  CHECK(basic.find("srgbToLinear(texelDiffuse)") != std::string::npos);
  CHECK(ground.find("srgbToLinear(texel)") != std::string::npos);
  // Normal-map data never decoded.
  CHECK(ground.find("srgbToLinear(texelNormal)") == std::string::npos);
  // Sky and sun cubemap/diffuse decoded as well.
  CHECK(sky.find("srgbToLinear(texture(texCube") != std::string::npos);
  CHECK(sun.find("vec3(2.2)") != std::string::npos);
}

// The old sky 1.4x gain double-lifted the sky after display gamma.
TEST_CASE("Sky has no brightness gain hack", "[render][color]")
{
  const std::string sky = ReadFile(ShaderDir() / "rt_skybox.fs");

  CHECK(sky.find("* 1.4") == std::string::npos);
  CHECK(sky.find("*1.4") == std::string::npos);
}
