#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

namespace {

std::string ReadShader(const std::filesystem::path &dir,
                       const std::string &name) {
  std::ifstream in(dir / name);
  REQUIRE(in.good());
  std::ostringstream body;
  body << in.rdbuf();
  return body.str();
}

} // namespace

// Phase 3 unified lit material: linear workflow (no gamma hacks, Kd <= 1),
// direction vectors with w == 0, per-material alphaCutoff, view-depth fog
// input, and the Phase 2 shadow contract untouched.
TEST_CASE("Phase3 rt_basic unified lit material", "[render][material]")
{
  const std::filesystem::path shaderDir =
      std::filesystem::path(JUNGLEIN_DATA_DIR) / "shaders";
  REQUIRE(std::filesystem::is_directory(shaderDir));

  const std::string vs = ReadShader(shaderDir, "rt_basic.vs");
  const std::string fs = ReadShader(shaderDir, "rt_basic.fs");

  // VS: direction vectors carry no translation.
  CHECK(vs.find("normalMatrix * normal") != std::string::npos);
  CHECK(vs.find("viewMatrix * lightDir") != std::string::npos);
  CHECK(vs.find("normalMatrix * normal, 1") == std::string::npos);
  CHECK(vs.find("viewMatrix * lightDir, 1") == std::string::npos);
  // VS: fog input comes from a view-depth varying (Phase 4 owns formula).
  // Instanced: per-instance matrices forbid a shared mv uniform, so the
  // view-space position is built from the view matrix and the instance
  // matrix instead (same value the old mv varying carried).
  CHECK(vs.find("vViewDepth") != std::string::npos);
  CHECK(vs.find("uniform mat4 mv") == std::string::npos);
  CHECK(vs.find("uniform mat4 view") != std::string::npos);
  CHECK(vs.find("in mat4 instanceModel") != std::string::npos);

  // FS: linear workflow, no gamma round-trip hacks.
  CHECK(fs.find("texelDiffuse * texelDiffuse") == std::string::npos);
  CHECK(fs.find("sqrt(") == std::string::npos);
  // FS: blowout fixed, principled double-sided lighting.
  CHECK(fs.find("Kd = 2.0") == std::string::npos);
  CHECK(fs.find("Kd = 1.0") != std::string::npos);
  CHECK(fs.find("ffcoef") == std::string::npos);
  CHECK(fs.find("gl_FrontFacing") != std::string::npos);
  // FS: single per-material alpha cutoff, no hardcoded threshold.
  CHECK(fs.find("alphaCutoff") != std::string::npos);
  CHECK(fs.find("< 0.8") == std::string::npos);
  // FS: stale-unit guards for optionally bound maps.
  CHECK(fs.find("useDiffuse") != std::string::npos);
  CHECK(fs.find("useAlpha") != std::string::npos);
  CHECK(fs.find("diffuseColor") != std::string::npos);
  // FS: fog reads the VS view-depth varying, not gl_FragCoord.
  CHECK(fs.find("vViewDepth") != std::string::npos);
  CHECK(fs.find("gl_FragCoord") == std::string::npos);
  // FS: light intensity plumbing intact.
  CHECK(fs.find("lightIntensity") != std::string::npos);
  // FS: Phase 2 shadow contract untouched.
  CHECK(fs.find("sampler2DShadow") != std::string::npos);
  CHECK(fs.find("textureProj(") != std::string::npos);
  CHECK(fs.find("textureSize(") != std::string::npos);
  CHECK(fs.find("0.005") != std::string::npos);
  CHECK(fs.find("0.0005") != std::string::npos);
  CHECK(fs.find("return 1.0") != std::string::npos);
  CHECK(fs.find("return 0.0") == std::string::npos);
}

TEST_CASE("Phase3 rt_ground_n unified lit material", "[render][material]")
{
  const std::filesystem::path shaderDir =
      std::filesystem::path(JUNGLEIN_DATA_DIR) / "shaders";
  REQUIRE(std::filesystem::is_directory(shaderDir));

  const std::string vs = ReadShader(shaderDir, "rt_ground_n.vs");
  const std::string fs = ReadShader(shaderDir, "rt_ground_n.fs");

  // VS: normals via normalMatrix, light dir rotation-only, view depth.
  CHECK(vs.find("normalMatrix * ") != std::string::npos);
  CHECK(vs.find("viewMatrix * lightDir") != std::string::npos);
  CHECK(vs.find("vec4(lightDir, 1") == std::string::npos);
  CHECK(vs.find("vec4(normalize(normal), 1") == std::string::npos);
  CHECK(vs.find("vViewDepth") != std::string::npos);
  CHECK(vs.find("vViewPos") != std::string::npos);
  // VS: tangent frame built from the corrected view-space normal.
  CHECK(vs.find("computeTangentSpace(viewNormal)") != std::string::npos);

  // FS: fog reads the VS view-depth varying, not gl_FragCoord.
  CHECK(fs.find("vViewDepth") != std::string::npos);
  CHECK(fs.find("gl_FragCoord") == std::string::npos);
  // FS: specular view vector from view-space position, not object space.
  CHECK(fs.find("normalize(-vViewPos)") != std::string::npos);
  // FS: principled double-sided lighting.
  CHECK(fs.find("gl_FrontFacing") != std::string::npos);
  // FS: no gamma hacks, no blowout coefficients.
  CHECK(fs.find("sqrt(") == std::string::npos);
  CHECK(fs.find("Kd = 2.0") == std::string::npos);
  // FS: light intensity plumbing intact.
  CHECK(fs.find("lightIntensity") != std::string::npos);
  // FS: Phase 2 shadow contract untouched.
  CHECK(fs.find("sampler2DShadow") != std::string::npos);
  CHECK(fs.find("textureProj(") != std::string::npos);
  CHECK(fs.find("textureSize(") != std::string::npos);
  CHECK(fs.find("0.005") != std::string::npos);
  CHECK(fs.find("0.0005") != std::string::npos);
  CHECK(fs.find("return 1.0") != std::string::npos);
  CHECK(fs.find("return 0.0") == std::string::npos);
}
