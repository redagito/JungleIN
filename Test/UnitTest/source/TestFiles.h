#pragma once

// Shared file fixtures for tests: writes small uncompressed 24-bit BMPs
// so image tests need no binary assets in the repo.

#include <array>
#include <catch2/catch_test_macros.hpp>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace TestFiles {

inline void WriteU16(std::ofstream &out, std::uint16_t v) {
  out.put(static_cast<char>(v & 0xFF));
  out.put(static_cast<char>((v >> 8) & 0xFF));
}

inline void WriteU32(std::ofstream &out, std::uint32_t v) {
  out.put(static_cast<char>(v & 0xFF));
  out.put(static_cast<char>((v >> 8) & 0xFF));
  out.put(static_cast<char>((v >> 16) & 0xFF));
  out.put(static_cast<char>((v >> 24) & 0xFF));
}

// rowsTopDown[0] is the top row, each pixel is {r, g, b}.
inline void WriteBMP(const std::filesystem::path &path, unsigned width,
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

inline std::filesystem::path TempBMP(const std::string &name) {
  return std::filesystem::temp_directory_path() / (name + ".bmp");
}

} // namespace TestFiles
