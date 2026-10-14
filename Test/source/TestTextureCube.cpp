#include <array>
#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <vector>

#include "TestFiles.h"
#include "core/TextureCube.h"

using TestFiles::WriteBMP;

namespace {

std::string WriteCubeFaces(const std::string &dirName, unsigned width,
                           unsigned height, unsigned char gray) {
  const std::filesystem::path dir =
      std::filesystem::temp_directory_path() / dirName;
  std::filesystem::create_directories(dir);
  const std::vector<std::array<unsigned char, 3>> pixels(
      static_cast<size_t>(width) * height, {gray, gray, gray});
  for (const char *face : {"px", "nx", "py", "ny", "pz", "nz"})
    WriteBMP(std::filesystem::path(dir) / (std::string(face) + ".bmp"),
             width, height, pixels);
  return (dir / "").string() + "/";
}

void RemoveDir(const std::string &dirSlash) {
  std::filesystem::remove_all(
      std::filesystem::path(dirSlash).parent_path());
}

} // namespace

// All six cubemap faces must share dimensions: GL uploads every face
// with one fixed size, so a mismatch silently corrupts the cube.
// NOTE: cubes are intentionally leaked: ~TextureCube issues GL calls.
TEST_CASE("TextureCube requires uniform face dimensions",
          "[core][texturecube]")
{
  SECTION("uniform faces load")
  {
    const std::string dir = WriteCubeFaces("junglein_cube_ok", 2, 2, 128);
    TextureCube *cube = new TextureCube(dir, ".bmp");
    CHECK(cube != nullptr);
    RemoveDir(dir);
  }

  SECTION("mismatched face fails fast")
  {
    const std::string dir = WriteCubeFaces("junglein_cube_bad", 2, 2, 128);
    const std::vector<std::array<unsigned char, 3>> big(9, {1, 2, 3});
    WriteBMP(std::filesystem::path(dir) / "py.bmp", 3, 3, big);
    CHECK_THROWS_AS(new TextureCube(dir, ".bmp"), std::runtime_error);
    RemoveDir(dir);
  }
}
