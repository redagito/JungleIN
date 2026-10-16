#pragma once

#include <string>
#include <vector>

// DebugOverlay: FPS counter + frame profiling text overlay for F3.
//
// Text is rasterized with stb_truetype at 18px from exactly one file,
// the vendored data/fonts/DejaVuSansMono.ttf (same file on every
// machine and OS; stb is already required). No system font paths, no
// fallback chain: a missing or unrasterizable font throws at first use
// instead of degrading silently. The charset is all printable ASCII so
// overlay lines (FPS, key cheat-sheet, future stats) can spell anything;
// control characters and DEL render blank.
// setFontPath overrides the location (tests run with a different working
// directory and inject the absolute data-dir path); the app never calls
// it and always uses the repo-relative default.
//
// Atlas generation (buildAtlas/glyphPixel) is pure CPU so tests can cover
// it headless. GL upload happens once (lazily on first visible draw) into
// a single texture; all visible text draws in one draw call.
//
// When hidden, draw() issues zero GL calls, so the post chain output is
// unaffected. When visible, draw() saves and restores viewport, blend,
// depth-test, cull and binding state around the single blended pass.
//
// Extending the overlay later: keep the built-in FPS lines on top and add
// more lines below with setLines(). New printable characters need a
// charset entry (see kCharset); the stb path rasterizes them.
class DebugOverlay {
public:
  // Atlas cell size in pixels. Sized to hold an 18px raster of the
  // widest/tallest charset glyph in any of the candidate fonts.
  static constexpr int kGlyphW = 16;
  static constexpr int kGlyphH = 24;
  // The atlas is already ~18px glyphs, so blit 1:1 to the framebuffer.
  static constexpr int kPixelScale = 1;
  static constexpr int kMarginPx = 8;
  static constexpr int kLineGapPx = 2;

  // Printable ASCII: overlay lines (FPS, cheat-sheet, future stats)
  // can spell anything. Space renders blank (advance only); control
  // characters and DEL are unsupported and render blank too.
  static constexpr const char* kCharset = " !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~";

  // Index of c inside kCharset, or -1 when unsupported. Never throws.
  static int glyphIndex(char c);

  // Repo-relative font location used when no override is set.
  static std::string fontPath();
  // Overrides the font location (tests inject the absolute data-dir
  // path; the app never calls this). Takes effect on next raster load.
  static void setFontPath(const std::string& path);

  // True when pixel (x,y) of the glyph for c is inked. Out-of-range
  // coordinates and unsupported characters (including space) are blank.
  static bool glyphPixel(char c, int x, int y);

  // Builds the CPU-side atlas: one byte per pixel (0 or 255), row-major,
  // glyphs packed left to right in kCharset order. Sets outW/outH.
  static std::vector<unsigned char> buildAtlas(int& outW, int& outH);

  static int glyphCount();

  DebugOverlay();
  ~DebugOverlay();

  DebugOverlay(const DebugOverlay&) = delete;
  DebugOverlay& operator=(const DebugOverlay&) = delete;

  // Extra lines drawn below the built-in FPS lines. Only charset
  // characters are visible; everything else renders blank.
  void setLines(const std::vector<std::string>& lines);

  void setVisible(bool visible);
  void toggleVisible();
  bool isVisible() const;

  // Feeds one real frame dt (seconds) into the FPS EMA and refreshes the
  // built-in lines. EMA alpha 0.1 settles in ~20 frames.
  void update(double dtSeconds);
  double fps() const;
  double avgMs() const;

  // Draws all lines top-left with one draw call. No-op (zero GL calls)
  // while hidden. fbWidth/fbHeight is the real framebuffer size used for
  // the explicit full-window viewport. Must be called with a GL context
  // current; lazily uploads the atlas texture once.
  void draw(int fbWidth, int fbHeight);

private:
  void ensureGL();
  void buildVertices(int fbWidth, int fbHeight,
    std::vector<float>& out) const;

  bool visible_ = false;
  bool glReady_ = false;
  double avgMs_ = 0.0;
  bool hasAvg_ = false;
  std::vector<std::string> extraLines_;
  // Reused vertex staging: buildVertices resizes (never shrinks
  // capacity), so frames stop paying vector growth churn.
  std::vector<float> vertsCache_;
  unsigned int texture_ = 0;
  unsigned int vao_ = 0;
  unsigned int vbo_ = 0;
  int atlasW_ = 0;
  int atlasH_ = 0;
};
