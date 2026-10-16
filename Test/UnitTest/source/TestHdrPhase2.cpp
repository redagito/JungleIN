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

// HDR Phase 2: ACES filmic tonemap (Narkowicz fit) runs after exposure
// and before gamma; the CSB/vignette looks stay display-referred
// between tonemap and gamma.
TEST_CASE("Display tonemaps with ACES after exposure before gamma",
          "[render][hdr]") {
  const std::string display = ReadFile(ShaderDir() / "ss_display.fs");

  // Narkowicz fit constants.
  CHECK(display.find("ACES") != std::string::npos);
  CHECK(display.find("2.51") != std::string::npos);
  CHECK(display.find("2.43") != std::string::npos);

  const size_t mainPos = display.find("void main()");
  REQUIRE(mainPos != std::string::npos);
  const size_t exposed = display.find("rgb * exposure", mainPos);
  const size_t aces = display.find("ACESFilm(", mainPos);
  const size_t gamma = display.find("GammaCorrection(", mainPos);
  REQUIRE(exposed != std::string::npos);
  REQUIRE(aces != std::string::npos);
  REQUIRE(gamma != std::string::npos);
  CHECK(exposed < aces);
  CHECK(aces < gamma);

  // Looks stay between tonemap and gamma.
  const size_t looks = display.find("ContrastSaturationBrightness(mapped");
  CHECK(looks != std::string::npos);
  CHECK(aces < looks);
}
