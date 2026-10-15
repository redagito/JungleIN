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

} // namespace

// Phase 2 Lights+Shadows real-fix: shadow maps use hardware compare
// (GL_COMPARE_REF_TO_TEXTURE), so lit shaders must sample with
// sampler2DShadow + textureProj, never manual texture().z.
TEST_CASE("Phase2 shadow shaders use hardware compare", "[render][shadows]")
{
  const std::filesystem::path shaderDir =
      std::filesystem::path(JUNGLEIN_DATA_DIR) / "shaders";
  REQUIRE(std::filesystem::is_directory(shaderDir));

  // Phase 3 deleted the unused rt_ground pair (MaterialGround binds
  // rt_ground_n); the remaining lit shaders keep the Phase 2 contract.
  for (const char *name : {"rt_basic", "rt_ground_n"}) {
    INFO("shader: " << name);
    const std::string fs = ReadFile(shaderDir / (std::string(name) + ".fs"));
    CHECK(fs.find("sampler2DShadow") != std::string::npos);
    CHECK(fs.find("textureProj(") != std::string::npos);
    CHECK(fs.find("textureSize(") != std::string::npos);
    // Manual depth fetch from a shadow map is broken under
    // GL_COMPARE_REF_TO_TEXTURE; hardware compare only.
    CHECK(fs.find("texture(texShadow,") == std::string::npos);
    CHECK(fs.find("texture(texShadowCascade,") == std::string::npos);
    // Slope bias replaces the constant 0.001 offset.
    CHECK(fs.find("0.005") != std::string::npos);
    CHECK(fs.find("0.0005") != std::string::npos);
    // Outside every frustum must be lit (1.0), never dark (0.0).
    CHECK(fs.find("return 1.0") != std::string::npos);
    CHECK(fs.find("return 0.0") == std::string::npos);
    // Perspective divide for the frustum test.
    CHECK(fs.find("/ fShadowcoord.w") != std::string::npos);
  }
}
