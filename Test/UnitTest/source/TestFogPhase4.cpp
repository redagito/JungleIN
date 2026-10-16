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
