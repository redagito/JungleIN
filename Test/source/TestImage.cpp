#include <array>
#include <catch2/catch_test_macros.hpp>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <vector>

#include "core/Image.h"

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

// Writes an uncompressed 24-bit BMP. rowsTopDown[0] is the top row,
// each pixel is {r, g, b}.
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
  WriteU32(out, 54 + pixelBytes); // file size
  WriteU32(out, 0);               // reserved
  WriteU32(out, 54);              // pixel offset
  WriteU32(out, 40);              // DIB size
  WriteU32(out, width);
  WriteU32(out, height); // positive height: rows stored bottom-up
  WriteU16(out, 1);      // planes
  WriteU16(out, 24);     // bits per pixel
  WriteU32(out, 0);      // no compression
  WriteU32(out, pixelBytes);
  WriteU32(out, 0); // h resolution
  WriteU32(out, 0); // v resolution
  WriteU32(out, 0); // colors
  WriteU32(out, 0); // important colors

  std::vector<char> padding(rowStride - width * 3u, 0);
  for (int y = static_cast<int>(height) - 1; y >= 0; --y) {
    for (unsigned x = 0; x < width; ++x) {
      const auto &px = rowsTopDown[static_cast<size_t>(y) * width + x];
      out.put(static_cast<char>(px[2])); // BMP stores BGR
      out.put(static_cast<char>(px[1]));
      out.put(static_cast<char>(px[0]));
    }
    out.write(padding.data(), static_cast<std::streamsize>(padding.size()));
  }
}

std::filesystem::path TempBMP(const std::string &name) {
  return std::filesystem::temp_directory_path() / (name + ".bmp");
}

} // namespace

TEST_CASE("Image pixel addressing", "[core][image]")
{
  const unsigned kW = 3;
  const unsigned kH = 2;
  std::vector<std::array<unsigned char, 3>> pixels;
  for (unsigned y = 0; y < kH; ++y)
    for (unsigned x = 0; x < kW; ++x)
      pixels.push_back({static_cast<unsigned char>(x * 80),
                        static_cast<unsigned char>(y * 120),
                        static_cast<unsigned char>(x * 40 + y * 60)});

  const auto file = TempBMP("junglein_stride");
  WriteBMP(file, kW, kH, pixels);

  SECTION("getPixel matches the decoded RGBA buffer layout")
  {
    const Image img(file.string(), false);
    REQUIRE(img.getWidth() == kW);
    REQUIRE(img.getHeight() == kH);
    const unsigned char *data = img.getData();
    for (unsigned y = 0; y < kH; ++y) {
      for (unsigned x = 0; x < kW; ++x) {
        const size_t i = (static_cast<size_t>(y) * kW + x) * 4;
        const Image::RGBA px = img.getPixel(x, y);
        CHECK(px.r == data[i]);
        CHECK(px.g == data[i + 1]);
        CHECK(px.b == data[i + 2]);
        CHECK(px.a == data[i + 3]);
      }
    }
  }

  SECTION("second row reads its own texels, not row zero")
  {
    const Image img(file.string(), false);
    // Authored row 1 has g=120, row 0 has g=0.
    CHECK(img.getPixel(0, 1).g == 120);
    CHECK(img.getPixel(2, 1).g == 120);
    CHECK(img.getPixel(0, 0).g == 0);
  }

  SECTION("out-of-range coordinates fail fast")
  {
    const Image img(file.string(), false);
    CHECK_THROWS_AS(img.getPixel(kW, 0), std::out_of_range);
    CHECK_THROWS_AS(img.getPixel(0, kH), std::out_of_range);
    CHECK_THROWS_AS(img.getPixel(99, 99), std::out_of_range);
  }

  SECTION("flip option controls which file row is row zero")
  {
    const auto flipFile = TempBMP("junglein_flip");
    // Top row red/green, bottom row blue/white.
    const std::vector<std::array<unsigned char, 3>> flipPixels = {
        {255, 0, 0}, {0, 255, 0}, {0, 0, 255}, {255, 255, 255}};
    WriteBMP(flipFile, 2, 2, flipPixels);

    const Image plain(flipFile.string(), false);
    CHECK(plain.getPixel(0, 0).r == 255);
    CHECK(plain.getPixel(0, 0).b == 0);
    CHECK(plain.getPixel(0, 1).b == 255);

    const Image flipped(flipFile.string(), true);
    CHECK(flipped.getPixel(0, 0).b == 255);
    CHECK(flipped.getPixel(0, 1).r == 255);
  }

  SECTION("missing file fails fast in the constructor")
  {
    CHECK_THROWS_AS(Image("does-not-exist-junglein.bmp"),
                    std::runtime_error);
  }

  std::filesystem::remove(file);
}
