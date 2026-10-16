#include <array>
#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <stdexcept>
#include <vector>

#include "TestFiles.h"
#include "core/Image.h"

using TestFiles::TempBMP;
using TestFiles::WriteBMP;

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
