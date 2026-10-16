// Batch-B fixtures: item 2 (SceneBuilder vegetation table snapshot) and
// item 3 (Ground vertex-normal fixtures).
//
// Full SceneBuilder::build and real Ground construction need GL
// (Texture::init, Shader::load, glDeleteTextures at teardown) plus
// registered shaders, so these fixtures raise a hidden context like
// TestShaderCompile instead of running purely headless. They also run
// with the repo root as CWD: SceneBuilder and Ground use relative
// data/... paths.
//
// A full Ground still cannot yield analytic tilted-plane normals: the
// radial poolEffect multiplies byte-quantized heights, so even exact
// pixel math lands ~1e-3 off the plane. The tilted-plane case therefore
// pins the extracted Ground::accumulateVertexNormals seam directly; the
// flat case (all-black heightmap forces height 0 everywhere, pool and
// all) goes through the real constructor.

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <array>
#include <cmath>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include "../../UnitTest/source/TestFiles.h"
#include "core/Texture.h"
#include "helpers/SceneBuilder.h"
#include "materials/MaterialGround.h"
#include "objects/Ground.h"
#include "objects/Mesh.h"
#include "objects/Scene.h"
#include "render/ShaderLibrary.h"

using Catch::Matchers::WithinAbs;
using TestFiles::WriteBMP;

namespace {

struct HiddenGL {
  GLFWwindow *window = nullptr;
  bool ok = false;

  HiddenGL() {
    if (glfwInit() == 0)
      return;
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    window = glfwCreateWindow(1, 1, "JungleINBatchB", nullptr, nullptr);
    if (window == nullptr) {
      glfwTerminate();
      return;
    }
    glfwMakeContextCurrent(window);
    ok = gladLoadGL((GLADloadfunc)glfwGetProcAddress) != 0;
    // Fresh library per context: static Shader objects outlive their
    // per-test GL context, so a later case would reuse programs compiled
    // for a dead context (silent empty draws). See resetActiveShader.
    if (ok)
      ShaderLibrary::clear();
  }

  ~HiddenGL() {
    if (window != nullptr)
      glfwDestroyWindow(window);
    if (ok)
      glfwTerminate();
  }
};

// SceneBuilder and Ground open relative data/... paths. Point CWD at the
// repo root (parent of JUNGLEIN_DATA_DIR) and restore on scope exit.
struct RepoRootCwd {
  std::filesystem::path saved;
  RepoRootCwd() {
    saved = std::filesystem::current_path();
    std::filesystem::current_path(
        std::filesystem::path(JUNGLEIN_DATA_DIR).parent_path());
  }
  ~RepoRootCwd() {
    std::error_code ec;
    std::filesystem::current_path(saved, ec);
  }
};

// Shaders SceneBuilder::build needs, with the exact attribute/uniform
// lists from Renderer::loadShaders (Renderer.cpp is owned by another
// worker; the lists are duplicated here so the fixture stands alone).
// Absolute prefixes so only the CWD (for data assets) matters.
void RegisterBuildShaders() {
  const std::string dir = std::string(JUNGLEIN_DATA_DIR) + "/shaders/";
  ShaderLibrary::addShader(
      "rt_basic", dir + "rt_basic",
      {"position", "normal", "texcoord"},
      {"mvp", "mv", "normalMatrix", "viewMatrix", "lightDir",
       "lightIntensity", "texDiffuse", "texAlpha", "alphaCutoff",
       "useDiffuse", "useAlpha", "diffuseColor", "shadowQuality",
       "texShadow", "shadowMatrix", "texShadowCascade",
       "shadowMatrixCascade", "cascadeShadow"});
  ShaderLibrary::addShader("rt_sun", dir + "rt_sun",
                           {"position", "texcoord"},
                           {"mvp", "texDiffuse", "texAlpha"});
  ShaderLibrary::addShader(
      "rt_ground_n", dir + "rt_ground_n", {"position", "normal"},
      {"mvp", "mv", "normalMatrix", "viewMatrix", "shadowMatrix",
       "shadowMatrixCascade", "cascadeShadow", "lightDir",
       "lightIntensity", "heightRange", "tileSize", "texMoss", "texEarth",
       "texShatter", "texNormalMoss", "texNormalEarth",
       "texNormalShatter", "shadowQuality", "texShadow",
       "texShadowCascade"});
  ShaderLibrary::addShader("rt_skybox", dir + "rt_skybox", {"position"},
                           {"mvp", "texCube"});
}

// Real Ground with CPU-only textures (index ctor: no image, no GL until
// init, which Ground never calls). Ownership transfers to the caller.
std::unique_ptr<Ground> MakeTestGround(const std::string &heightmap) {
  auto moss = std::make_unique<Texture>(0u);
  auto earth = std::make_unique<Texture>(1u);
  auto shatter = std::make_unique<Texture>(2u);
  auto mossN = std::make_unique<Texture>(3u);
  auto earthN = std::make_unique<Texture>(4u);
  auto shatterN = std::make_unique<Texture>(5u);
  auto mat = std::make_unique<MaterialGround>(std::move(moss),
    std::move(earth), std::move(shatter), std::move(mossN),
    std::move(earthN), std::move(shatterN));
  return std::make_unique<Ground>(heightmap, std::move(mat));
}

void CheckVec(const glm::vec3 &v, float x, float y, float z) {
  CHECK_THAT(v.x, WithinAbs(x, 1e-6f));
  CHECK_THAT(v.y, WithinAbs(y, 1e-6f));
  CHECK_THAT(v.z, WithinAbs(z, 1e-6f));
}

} // namespace

// Item 2 snapshot: SceneBuilder::build vegetation stanzas as of 6045fd2.
// Requested instances per mesh are lround(1400 * pourcentage):
// rock 0.20 -> 280, palm 0.07 -> 98, htree 0.30 -> 420,
// btree 0.15 -> 210, gpalm 0.14 -> 196, bush 0.14 -> 196; total 1400.
// The table refactor must preserve every value below.
TEST_CASE("SceneBuilder build vegetation snapshot", "[batch-b][scenebuilder]")
{
  HiddenGL gl;
  if (!gl.ok)
    SKIP("No GL context available for SceneBuilder build test");
  RepoRootCwd cwd;
  RegisterBuildShaders();

  std::unique_ptr<Scene> scene = SceneBuilder::build();
  REQUIRE(scene != nullptr);
  REQUIRE(SceneBuilder::NB_INSTANCE == 1400);

  const std::vector<Mesh *> meshes = scene->getMeshes();
  REQUIRE(meshes.size() == 6u);
  REQUIRE(scene->getCameras().size() == 1u);
  CHECK(scene->getSun() != nullptr);
  CHECK(scene->getSky() != nullptr);
  CHECK(scene->getGround() != nullptr);

  struct Expect {
    unsigned int type;
    float pct;
    float scaleRdn;
    float heightRdn;
    float range;
    glm::vec3 pos;
    int instances;
  };
  // Scene order: rock1, palm, gpalm, htree, btree, bush1.
  const std::vector<Expect> want = {
      {Mesh::INSTANCE_ROCK, 0.20f, 0.2f, 0.5f, 0.4f,
       glm::vec3(-3.0f, 0.5f, 0.0f), 280},
      {Mesh::INSTANCE_PALM, 0.07f, 0.8f, -2.5f, 0.8f,
       glm::vec3(0.0f, 0.0f, -10.0f), 98},
      {Mesh::INSTANCE_GPALM, 0.14f, 0.9f, -1.0f, 0.4f,
       glm::vec3(-3.0f, -1.0f, 0.0f), 196},
      {Mesh::INSTANCE_HTREE, 0.30f, 0.3f, -2.0f, 0.4f,
       glm::vec3(-6.0f, 2.0f, -16.0f), 420},
      {Mesh::INSTANCE_BTREE, 0.15f, 0.3f, -2.0f, 1.0f,
       glm::vec3(-4.0f, -2.0f, 4.0f), 210},
      {Mesh::INSTANCE_BUSH, 0.14f, 0.3f, 0.0f, 0.4f,
       glm::vec3(-3.0f, 0.0f, 0.0f), 196},
  };

  int total = 0;
  for (size_t i = 0; i < want.size(); ++i) {
    INFO("mesh index " << i);
    CHECK(meshes[i]->getInstanceType() == want[i].type);
    CHECK_THAT(meshes[i]->getPourcentage(), WithinAbs(want[i].pct, 1e-6f));
    CHECK_THAT(meshes[i]->getScaleRdn(), WithinAbs(want[i].scaleRdn, 1e-6f));
    CHECK_THAT(meshes[i]->getHeightRdn(),
               WithinAbs(want[i].heightRdn, 1e-6f));
    CHECK_THAT(meshes[i]->getRangeScale(), WithinAbs(want[i].range, 1e-6f));
    CheckVec(meshes[i]->getPosition(), want[i].pos.x, want[i].pos.y,
             want[i].pos.z);
    CHECK(meshes[i]->getInstances().size() ==
          static_cast<size_t>(want[i].instances));
    total += static_cast<int>(meshes[i]->getInstances().size());
  }
  CHECK(total == 1400);

  scene.reset();
  SceneBuilder::scene2 = nullptr;
  SceneBuilder::ground2 = nullptr;
  ShaderLibrary::clear();
}

// Item 3, tilted-plane case against the Ground::accumulateVertexNormals
// seam (see Ground.h): a full Ground cannot hold an analytic plane
// (radial poolEffect times byte quantization bends it ~1e-3), so the
// indexed pass is pinned directly with synthetic faces.
//
// Setup mirrors Ground's own triangulation for a 3x3 grid (vertex
// v(x,z) = x + z*3; per quad: (h,ver,h+1) and (h+1,ver,ver+1)), giving
// face order f0..f7 over quads (0,0),(1,0),(0,1),(1,1).
namespace {

std::vector<int> TiltGridIndices() {
  // quads (0,0): h=0 ver=3 | (1,0): h=1 ver=4 | (0,1): h=3 ver=6 |
  // (1,1): h=4 ver=7
  return {0, 3, 1, 1, 3, 4, 1, 4, 2, 2, 4, 5,
          3, 6, 4, 4, 6, 7, 4, 7, 5, 5, 7, 8};
}

void CheckNormalNear(const float *out, int v, const glm::vec3 &ref) {
  INFO("vertex " << v);
  CHECK_THAT(out[v * 3], WithinAbs(ref.x, 1e-5f));
  CHECK_THAT(out[v * 3 + 1], WithinAbs(ref.y, 1e-5f));
  CHECK_THAT(out[v * 3 + 2], WithinAbs(ref.z, 1e-5f));
}

} // namespace

TEST_CASE("Ground accumulateVertexNormals tilted plane", "[batch-b][ground]")
{
  const std::vector<int> indices = TiltGridIndices();
  constexpr int kVerts = 9;

  SECTION("single-slope plane matches the analytic normal")
  {
    // Plane y = 0.5 x has unit normal (-0.5, 1, 0) / |..|; every
    // single-slope quad in Ground yields that same unit face normal.
    const glm::vec3 ref = glm::normalize(glm::vec3(-0.5f, 1.0f, 0.0f));
    const std::vector<glm::vec3> faces(8, ref);
    std::vector<float> out(static_cast<size_t>(kVerts * 3), 0.0f);
    Ground::accumulateVertexNormals(indices.data(),
                                    static_cast<int>(indices.size()),
                                    faces.data(), kVerts, out.data());
    for (int v = 0; v < kVerts; ++v)
      CheckNormalNear(out.data(), v, ref);
  }

  SECTION("each adjacent face is counted once")
  {
    // Alternating faces: corners see one face, edges two or three, the
    // center six. The double-precision loop below re-derives the
    // expectation in reverse triangle order, so shared-face
    // double-counts or dropped faces fail.
    const glm::vec3 n1(0.0f, 1.0f, 0.0f);
    const glm::vec3 n2 = glm::normalize(glm::vec3(0.5f, 1.0f, 0.25f));
    std::vector<glm::vec3> faces;
    for (int t = 0; t < 8; ++t)
      faces.push_back(t % 2 == 0 ? n1 : n2);
    std::vector<float> out(static_cast<size_t>(kVerts * 3), 0.0f);
    Ground::accumulateVertexNormals(indices.data(),
                                    static_cast<int>(indices.size()),
                                    faces.data(), kVerts, out.data());

    // Closed-form spot check: the center vertex touches f1..f6, i.e.
    // three of each, and 3*n1 + 3*n2 normalizes like n1 + n2.
    const glm::vec3 center = glm::normalize(n1 + n2);
    CheckNormalNear(out.data(), 4, center);

    for (int v = 0; v < kVerts; ++v) {
      glm::dvec3 acc(0.0);
      for (int t = 7; t >= 0; --t)
        for (int k = 0; k < 3; ++k)
          if (indices[static_cast<size_t>(t * 3 + k)] == v)
            acc += glm::dvec3(faces[static_cast<size_t>(t)]);
      acc = glm::normalize(acc);
      CheckNormalNear(out.data(), v, glm::vec3(acc));
    }
  }
}

// Item 3, flat case through the real constructor: an all-black heightmap
// forces every height to MAX_HEIGHT * pool * 0 = 0, so the ground is an
// exact plane and every normal must read +Y (x/z bitwise zero, y within
// float noise of 1).
TEST_CASE("Ground flat heightmap normals point up", "[batch-b][ground]")
{
  HiddenGL gl;
  if (!gl.ok)
    SKIP("No GL context available for Ground fixture test");
  RepoRootCwd cwd;
  RegisterBuildShaders();

  const unsigned kN = 8;
  std::vector<std::array<unsigned char, 3>> black(kN * kN, {0, 0, 0});
  const auto file =
      std::filesystem::temp_directory_path() / "junglein_batchb_flat.bmp";
  WriteBMP(file, kN, kN, black);

  Ground *ground = MakeTestGround(file.string()).release();
  const float *normals = ground->getGeometry()->getNormals();
  REQUIRE(ground->getGeometry()->getVertexFloatCount() ==
          static_cast<int>(kN * kN * 3));
  for (unsigned v = 0; v < kN * kN; ++v) {
    INFO("vertex " << v);
    CHECK(normals[v * 3] == 0.0f);
    CHECK_THAT(normals[v * 3 + 1], WithinAbs(1.0f, 1e-6f));
    CHECK(normals[v * 3 + 2] == 0.0f);
  }

  delete ground;
  ShaderLibrary::clear();
  std::filesystem::remove(file);
}

// Item 3, real-data smoke: the shipped heightmap must yield finite,
// upward normals both before and after the replacement. Magnitude is
// deliberately NOT pinned: the old code stores the raw face average
// (observed |n|^2 down to ~0.3 on rough ground), while the indexed pass
// normalizes to |n| == 1. Direction (+Y) is the invariant.
TEST_CASE("Ground shipped heightmap normals stay sane", "[batch-b][ground]")
{
  HiddenGL gl;
  if (!gl.ok)
    SKIP("No GL context available for Ground fixture test");
  RepoRootCwd cwd;
  RegisterBuildShaders();

  Ground *ground = MakeTestGround(std::string(JUNGLEIN_DATA_DIR) +
                                   "/resources/heightmaps/heightmap.png").release();
  const float *normals = ground->getGeometry()->getNormals();
  const int count = ground->getGeometry()->getVertexFloatCount();
  REQUIRE(count == 257 * 257 * 3);
  for (int v = 0; v < count; v += 3) {
    const float nx = normals[v];
    const float ny = normals[v + 1];
    const float nz = normals[v + 2];
    INFO("vertex " << v / 3);
    CHECK(std::isfinite(nx));
    CHECK(std::isfinite(ny));
    CHECK(std::isfinite(nz));
    CHECK(ny > 0.0f);
  }

  delete ground;
  ShaderLibrary::clear();
}
