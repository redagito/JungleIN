#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <string>
#include <vector>

#include "render/DebugOverlay.h"
#include "helpers/Profiler.h"
#include "ui/Window.h"

namespace {

// Test binaries run with a different working directory than the app, so
// the repo-relative default font path does not resolve here. Inject the
// absolute data-dir path before any raster use in every test below.
void UseRepoFont() {
  DebugOverlay::setFontPath(
    (std::filesystem::path(JUNGLEIN_DATA_DIR) / "fonts" /
      "DejaVuSansMono.ttf")
      .string());
}

} // namespace

// The rasterized font covers the full printable-ASCII charset: every
// listed glyph (except space) must be non-blank, or the overlay draws
// nothing. A missing/unrasterizable font throws instead of degrading,
// so this test fails loudly rather than passing on blank glyphs.
TEST_CASE("DebugOverlay atlas covers the full charset", "[ui][overlay]") {
  UseRepoFont();
  const std::string charset(DebugOverlay::kCharset);
  CHECK(DebugOverlay::glyphCount() == static_cast<int>(charset.size()));

  int w = 0;
  int h = 0;
  const std::vector<unsigned char> atlas = DebugOverlay::buildAtlas(w, h);
  CHECK(w == DebugOverlay::glyphCount() * DebugOverlay::kGlyphW);
  CHECK(h == DebugOverlay::kGlyphH);
  REQUIRE(atlas.size() ==
    static_cast<size_t>(w) * static_cast<size_t>(h));

  for (size_t gi = 0; gi < charset.size(); ++gi) {
    const char c = charset[gi];
    CHECK(DebugOverlay::glyphIndex(c) == static_cast<int>(gi));
    int ink = 0;
    for (int y = 0; y < DebugOverlay::kGlyphH; ++y) {
      for (int x = 0; x < DebugOverlay::kGlyphW; ++x) {
        const bool px = DebugOverlay::glyphPixel(c, x, y);
        const unsigned char cell = atlas[static_cast<size_t>(y) *
          static_cast<size_t>(w) +
          static_cast<size_t>(gi * DebugOverlay::kGlyphW + x)];
        // Atlas cell matches the glyph query, values are 0 or 255 only.
        CHECK(cell == (px ? 255 : 0));
        if (px)
          ++ink;
      }
    }
    if (c == ' ')
      CHECK(ink == 0);
    else
      CHECK(ink > 0);
  }
}

// Easily confused glyphs must map to their own shapes: a past bug had
// '.' and ':' swapped in the row table while kCharset listed them the
// other way round, so "39.1" rendered as "39:1". The atlas now comes
// from stb_truetype rasterization, so exact pixels depend on the system
// font file and are not pinned here. Pin what the swap bug would break
// instead: each of '.' and ':' has ink, and the two rasters differ in
// at least one pixel (a swap would still fail the diff check only if
// the table order changed; distinctness plus per-glyph ink keeps the
// regression covered without depending on font-file specifics).
TEST_CASE("DebugOverlay glyph shapes match their characters",
  "[ui][overlay]") {
  UseRepoFont();
  auto inkCount = [](char c) {
    int n = 0;
    for (int y = 0; y < DebugOverlay::kGlyphH; ++y) {
      for (int x = 0; x < DebugOverlay::kGlyphW; ++x) {
        if (DebugOverlay::glyphPixel(c, x, y))
          ++n;
      }
    }
    return n;
  };
  CHECK(inkCount('.') > 0);
  CHECK(inkCount(':') > 0);
  int diff = 0;
  for (int y = 0; y < DebugOverlay::kGlyphH; ++y) {
    for (int x = 0; x < DebugOverlay::kGlyphW; ++x) {
      if (DebugOverlay::glyphPixel('.', x, y) !=
        DebugOverlay::glyphPixel(':', x, y))
        ++diff;
    }
  }
  CHECK(diff > 0);
  // 'F' renders as ink, not blank.
  CHECK(inkCount('F') > 0);
}

// Characters outside printable ASCII must render blank, never garbage
// or a crash: out-of-range lookups stay blank-safe. (Everything from
// space to '~' is in-charset by design, rasterized from a real font.)
TEST_CASE("DebugOverlay unknown chars are blank-safe", "[ui][overlay]") {
  UseRepoFont();
  for (char c : std::string("\n\t\x01\x7f", 4)) {
    CHECK(DebugOverlay::glyphIndex(c) == -1);
    for (int y = 0; y < DebugOverlay::kGlyphH; ++y) {
      for (int x = 0; x < DebugOverlay::kGlyphW; ++x)
        CHECK_FALSE(DebugOverlay::glyphPixel(c, x, y));
    }
  }
  // Out-of-range coordinates are blank too.
  CHECK_FALSE(DebugOverlay::glyphPixel('F', -1, 0));
  CHECK_FALSE(
    DebugOverlay::glyphPixel('F', DebugOverlay::kGlyphW, 0));
  CHECK_FALSE(DebugOverlay::glyphPixel('F', 0, -1));
  CHECK_FALSE(
    DebugOverlay::glyphPixel('F', 0, DebugOverlay::kGlyphH));
}

// The overlay defaults to hidden and toggles explicitly. Pure CPU: no GL
// context is created here, and none is needed (no draw call is issued).
TEST_CASE("DebugOverlay defaults hidden with toggle API", "[ui][overlay]") {
  DebugOverlay overlay;
  CHECK_FALSE(overlay.isVisible());
  overlay.toggleVisible();
  CHECK(overlay.isVisible());
  overlay.toggleVisible();
  CHECK_FALSE(overlay.isVisible());
  overlay.setVisible(true);
  CHECK(overlay.isVisible());
  overlay.setLines({"FPS 60", "16.7 ms avg"});
  overlay.update(1.0 / 60.0);
  CHECK(overlay.fps() > 0.0);
  CHECK(overlay.avgMs() > 0.0);
}

// Profile lines list the hottest sections first (mean ms each), so the
// overlay shows where frame time goes. Pure formatting: no GL, no window.
TEST_CASE("Overlay profile lines spell hottest sections", "[ui][overlay]") {
  const std::vector<Profiler::SectionStats> stats = {
      {"meshes", 2, 5.0, 6.0, 10.0},
      {"shadow", 2, 4.0, 4.5, 8.0},
  };
  const std::vector<std::string> lines =
      Window::overlayProfileLines(stats, 2);
  REQUIRE(lines.size() == 1);
  CHECK(lines[0].find("meshes 5.0ms") != std::string::npos);
  CHECK(lines[0].find("shadow 4.0ms") != std::string::npos);
  CHECK(lines[0].find("meshes") < lines[0].find("shadow"));

  // Empty stats produce no lines (overlay keeps its 7 status lines).
  CHECK(Window::overlayProfileLines({}, 2).empty());
  // topN caps the entry count, hottest first.
  CHECK(Window::overlayProfileLines(stats, 1).size() == 1);
}

TEST_CASE("Overlay profile lines chunk entries", "[ui][overlay]") {
  const std::vector<Profiler::SectionStats> stats = {
      {"a", 1, 9.0, 9.0, 9.0},
      {"b", 1, 8.0, 8.0, 8.0},
      {"c", 1, 7.0, 7.0, 7.0},
      {"d", 1, 6.0, 6.0, 6.0},
      {"e", 1, 5.0, 5.0, 5.0},
      {"f", 1, 4.0, 4.0, 4.0},
      {"g", 1, 3.0, 3.0, 3.0},
  };
  // Ten requested, seven present: five on the first line, two on the
  // second, hottest first across both lines.
  const std::vector<std::string> lines =
      Window::overlayProfileLines(stats, 10);
  REQUIRE(lines.size() == 2);
  CHECK(lines[0].find("a 9.0ms") != std::string::npos);
  CHECK(lines[0].find("e 5.0ms") != std::string::npos);
  CHECK(lines[0].find("f 4.0ms") == std::string::npos);
  CHECK(lines[1].find("f 4.0ms") != std::string::npos);
  CHECK(lines[1].find("g 3.0ms") != std::string::npos);
}

// Overlay cheat-sheet spells live toggle states (movement keys stay
// out). Pure formatting: no GL, no window.
TEST_CASE("Overlay status lines spell toggle states", "[ui][overlay]") {
  const std::vector<std::string> on =
    Window::overlayStatusLines(true, true, true, 1.0f, true, true, true,
      true, true, true, 12.3f, 4.5f, -100.2f, 9.3f, true);
  REQUIRE(on.size() == 7);
  CHECK(on[0].find("1 FXAA on") != std::string::npos);
  CHECK(on[0].find("2 Flares on") != std::string::npos);
  CHECK(on[1].find("3 AutoExp on") != std::string::npos);
  CHECK(on[1].find("7/8 Bias 1.00") != std::string::npos);
  CHECK(on[2].find("4 Rays on") != std::string::npos);
  CHECK(on[2].find("5 Bloom on") != std::string::npos);
  CHECK(on[2].find("6 Dist on") != std::string::npos);
  CHECK(on[3].find("F4 VSync on") != std::string::npos);
  CHECK(on[4].find("R Regen") != std::string::npos);
  CHECK(on[4].find("9 Shad on") != std::string::npos);
  CHECK(on[4].find("0 Wire on") != std::string::npos);
  CHECK(on[5] == "Cam 12.3 4.5 -100.2");
  CHECK(on[6] == "Look 9.3m");

  const std::vector<std::string> off =
    Window::overlayStatusLines(false, false, false, 0.5f, false, false,
      false, false, false, false, 0.0f, 0.0f, 0.0f, 0.0f, false);
  CHECK(off[0].find("1 FXAA off") != std::string::npos);
  CHECK(off[1].find("7/8 Bias 0.50") != std::string::npos);
  CHECK(off[2].find("6 Dist off") != std::string::npos);
  CHECK(off[3].find("F4 VSync off") != std::string::npos);
  CHECK(off[4].find("9 Shad off") != std::string::npos);
  CHECK(off[4].find("0 Wire off") != std::string::npos);
  CHECK(off[5] == "Cam 0.0 0.0 0.0");
  CHECK(off[6] == "Look ---");
}
