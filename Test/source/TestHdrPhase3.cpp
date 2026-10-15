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

// HDR Phase 3: RGBA16F render targets must declare GL_HALF_FLOAT.
// GL_UNSIGNED_BYTE only worked because uploads pass NULL data; the
// depth path (GL_FLOAT) and image uploads (byte path) stay untouched.
TEST_CASE("HDR targets upload as half float", "[render][hdr]") {
  const std::string textureCpp =
      ReadFile(RepoRoot() / "Lib" / "source" / "core" / "Texture.cpp");

  CHECK(textureCpp.find("GL_HALF_FLOAT") != std::string::npos);
  CHECK(textureCpp.find("GL_RGBA16F") != std::string::npos);
  // Depth and byte paths preserved.
  CHECK(textureCpp.find("GL_FLOAT") != std::string::npos);
  CHECK(textureCpp.find("GL_UNSIGNED_BYTE") != std::string::npos);
}

// HDR Phase 3: auto-exposure toggle + exposure bias are wired through
// Window like the other post toggles (keys 5/6 on/off, 7/8 bias
// down/up), following the 1/2 FXAA and 3/4 flare key pattern.
TEST_CASE("Window exposes auto-exposure controls", "[ui][hdr]") {
  const std::filesystem::path root = RepoRoot();
  const std::string windowCpp =
      ReadFile(root / "Lib" / "source" / "ui" / "Window.cpp");
  const std::string windowH =
      ReadFile(root / "Lib" / "include" / "ui" / "Window.h");

  CHECK(windowH.find("exPostAutoExposure") != std::string::npos);
  CHECK(windowH.find("exExposureBias") != std::string::npos);
  CHECK(windowCpp.find("toggleAutoExposure") != std::string::npos);
  CHECK(windowCpp.find("setExposureBias") != std::string::npos);
  CHECK(windowCpp.find("GLFW_KEY_5") != std::string::npos);
  CHECK(windowCpp.find("GLFW_KEY_6") != std::string::npos);
  CHECK(windowCpp.find("GLFW_KEY_7") != std::string::npos);
  CHECK(windowCpp.find("GLFW_KEY_8") != std::string::npos);
}

// HDR Phase 3: godrays/lensflare deliberately unchanged. Both sample
// texThreshold, which only the sun writes (every other rt_* writes
// black), and the sun texture peaks at 1.0, so the flare 0.5 subtract
// threshold and the godray sampling weights see the same input range
// as before ACES. Locked structurally, not numerically, so a future
// visual pass can retune freely.
TEST_CASE("Flare and godrays keep their pre-tonemap weights",
          "[render][hdr]") {
  const std::filesystem::path root = RepoRoot();
  const std::string composer =
      ReadFile(root / "Lib" / "source" / "render" / "PostComposer.cpp");
  const std::string flare = ReadFile(ShaderDir() / "ss_lenseflare_p1.fs");
  const std::string godrays = ReadFile(ShaderDir() / "ss_godrays_p1.fs");

  CHECK(composer.find("transmitUniform(\"flareThreshold\"") !=
        std::string::npos);
  CHECK(flare.find("texel - flareThreshold") != std::string::npos);
  CHECK(godrays.find("weight") != std::string::npos);
  CHECK(godrays.find("decay") != std::string::npos);
}
