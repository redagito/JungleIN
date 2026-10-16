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

// HDR Phase 3: bloom samples pre-tonemap linear HDR, so the threshold
// is scene-referred at 1.0 with a soft knee to 2.0. The LDR-era
// clamp(grey - 0.3, 0, 0.3) bloomed mid-tones; diffuse stays < 1.0 in
// linear HDR and only highlights exceed it.
TEST_CASE("Bloom threshold is scene-referred for linear HDR",
          "[render][hdr]") {
  const std::string bloom = ReadFile(ShaderDir() / "ss_bloom_p1.fs");

  CHECK(bloom.find("smoothstep(1.0, 2.0, grey)") != std::string::npos);
  // The old LDR threshold expression must be gone.
  CHECK(bloom.find("(grey) - 0.3") == std::string::npos);
  CHECK(bloom.find("0.0, 0.3") == std::string::npos);
}
