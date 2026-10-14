#include <array>
#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <vector>

#include "TestFiles.h"
#include "objects/Path.h"

using TestFiles::WriteBMP;

// Path masks follow the historical QImage top-left origin: pixel row 0
// is the top row of the file. validate() maps z=MAP_MIN to row 0.
TEST_CASE("Path mask orientation", "[objects][path]")
{
  // 2x2 mask: top row black (walkable), bottom row white (blocked).
  const std::vector<std::array<unsigned char, 3>> mask = {
      {0, 0, 0}, {0, 0, 0}, {255, 255, 255}, {255, 255, 255}};
  const auto file =
      std::filesystem::temp_directory_path() / "junglein_path.bmp";
  WriteBMP(file, 2, 2, mask);

  const Path path(file.string());
  // (-60,-60) samples pixel (0,0): black -> walkable.
  CHECK(path.validate(-60.0, -60.0));
  // (-60,60) samples pixel (0,1): white -> blocked.
  CHECK_FALSE(path.validate(-60.0, 60.0));

  std::filesystem::remove(file);
}
