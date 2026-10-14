#include <array>
#include <catch2/catch_test_macros.hpp>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <vector>

#include "objects/Path.h"

namespace {

void WriteU16(std::ofstream &out, std::uint16_t v) {
  out.put(static_cast<char>(v & 0xFF));
  out.put(static_cast<char>((v >> 8) & 0xFF));
}

void WriteU32(std::ofstream &out, std::uint32_t v) {
  out.put(static_cast<char>(v & 0xFF));
  out.put(static_cast<char>((v >> 8) & 0xFF));
  out.put(static_cast<char>((v >> 16) & 0xFF));
  out.put(static_cast<char>((v >> 24) & 0xFF));
}

void WriteBMP(const std::filesystem::path &path, unsigned width,
              unsigned height,
              const std::vector<std::array<unsigned char, 3>> &rowsTopDown) {
  REQUIRE(rowsTopDown.size() == static_cast<size_t>(width) * height);
  const unsigned rowStride = ((width * 3u + 3u) / 4u) * 4u;
  const std::uint32_t pixelBytes = rowStride * height;
  std::ofstream out(path, std::ios::binary);
  REQUIRE(out.good());
  out.put('B');
  out.put('M');
  WriteU32(out, 54 + pixelBytes);
  WriteU32(out, 0);
  WriteU32(out, 54);
  WriteU32(out, 40);
  WriteU32(out, width);
  WriteU32(out, height);
  WriteU16(out, 1);
  WriteU16(out, 24);
  WriteU32(out, 0);
  WriteU32(out, pixelBytes);
  WriteU32(out, 0);
  WriteU32(out, 0);
  WriteU32(out, 0);
  WriteU32(out, 0);
  std::vector<char> padding(rowStride - width * 3u, 0);
  for (int y = static_cast<int>(height) - 1; y >= 0; --y) {
    for (unsigned x = 0; x < width; ++x) {
      const auto &px = rowsTopDown[static_cast<size_t>(y) * width + x];
      out.put(static_cast<char>(px[2]));
      out.put(static_cast<char>(px[1]));
      out.put(static_cast<char>(px[0]));
    }
    out.write(padding.data(), static_cast<std::streamsize>(padding.size()));
  }
}

} // namespace

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
