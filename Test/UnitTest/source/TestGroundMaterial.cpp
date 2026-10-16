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

// Ground specular discipline: a broad white lobe on bumpy moss reads as
// chalk at grazing angles under exposure gain. The lobe is narrow and
// vegetation is near-matte; only rock shines.
TEST_CASE("Ground specular is narrow and per-band masked", "[render][ground]")
{
  const std::string fs =
      ReadFile(ShaderDir() / "rt_ground_n.fs");

  // Tight lobe, not the old spray-everywhere exponent.
  CHECK(fs.find("Shininess = 24.0") != std::string::npos);
  CHECK(fs.find("Shininess = 4.0") == std::string::npos);
  // Per-band response exists and moss is near-matte.
  CHECK(fs.find("SPEC_MOSS") != std::string::npos);
  CHECK(fs.find("SPEC_SHATTER") != std::string::npos);
  CHECK(fs.find("SPEC_EARTH") != std::string::npos);
  // The mask reaches the specular term and follows the albedo blends.
  CHECK(fs.find("specScale") != std::string::npos);
  CHECK(fs.find("mix(SPEC_SHATTER, SPEC_EARTH") != std::string::npos);
  CHECK(fs.find("mix(SPEC_EARTH, SPEC_MOSS") != std::string::npos);
  // Normal-map data still never decoded.
  CHECK(fs.find("srgbToLinear(texelNormal)") == std::string::npos);
}
