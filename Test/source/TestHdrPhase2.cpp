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

std::filesystem::path RepoRoot() {
  return std::filesystem::path(JUNGLEIN_DATA_DIR).parent_path();
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

// HDR Phase 2: exposure is driven from measured frame luminance with
// temporal adaptation; the manual knob is a bias on top of auto.
TEST_CASE("PostComposer drives exposure from luminance adaptation",
          "[render][hdr]") {
  const std::filesystem::path root = RepoRoot();
  const std::string composer =
      ReadFile(root / "Lib" / "source" / "render" / "PostComposer.cpp");
  const std::string composerH =
      ReadFile(root / "Lib" / "include" / "render" / "PostComposer.h");
  const std::string rendererCpp =
      ReadFile(root / "Lib" / "source" / "render" / "Renderer.cpp");

  // Luminance path: downsample target, one CPU readback, log-average
  // with Rec.709 luma and an eps guard.
  CHECK(composer.find("glReadPixels") != std::string::npos);
  CHECK(composer.find("log(") != std::string::npos);
  CHECK(composer.find("0.2126") != std::string::npos);
  CHECK(composer.find("0.7152") != std::string::npos);
  CHECK(composer.find("0.0722") != std::string::npos);
  // Temporal adaptation with asymmetric dark/bright time constants.
  CHECK(composer.find("exp(-dt") != std::string::npos);
  CHECK(composer.find("Tau") != std::string::npos);
  // Guards: NaN readback, sane luminance/exposure ranges.
  CHECK(composer.find("isfinite") != std::string::npos);
  CHECK(composer.find("clamp") != std::string::npos);

  // Manual knob becomes auto bias; both entry points exist.
  CHECK(composer.find("toggleAutoExposure") != std::string::npos);
  CHECK(composer.find("setExposureBias") != std::string::npos);
  CHECK(composerH.find("toggleAutoExposure") != std::string::npos);
  CHECK(composerH.find("setExposureBias") != std::string::npos);

  // Frame delta reaches the composer (no bare render() call left).
  CHECK(composerH.find("render(float") != std::string::npos);
  CHECK(rendererCpp.find("postComposer->render()") == std::string::npos);
  CHECK(rendererCpp.find("postComposer->render(") != std::string::npos);
}
