#include "render/DebugOverlay.h"

#include <cstdio>
#include <fstream>
#include <stdexcept>
#include <string>
#include <utility>

#include <glad/gl.h>

// stb_truetype implementation lives in this TU only, mirroring the
// STB_IMAGE_IMPLEMENTATION precedent in Lib/source/core/Image.cpp.
#define STB_TRUETYPE_IMPLEMENTATION
#include <stb_truetype.h>

#include "render/ShaderLibrary.h"

namespace {

// Raster pixel height for the stb path. The atlas already holds ~18px
// glyphs and kPixelScale is 1, so atlas pixels map 1:1 to the screen.
// The 16x24 cells hold every charset glyph box of every candidate font
// with room to spare; box bearings may still start a pixel outside the
// cell, which the blit clips (never rejects the font).
constexpr float kFontPixelHeight = 18.0f;
// Exactly one font file: the vendored repo asset. No system paths, no
// fallback chain - a missing font is a broken install and throws below
// instead of silently degrading to another machine's font.
constexpr const char* kDefaultFontPath = "data/fonts/DejaVuSansMono.ttf";
// stb coverage (0-255) at or above this becomes ink (255), else blank.
// Keeps the atlas binary so the shader's discard(<0.5) behaves as before
// and the CPU atlas stays exactly consistent with glyphPixel queries.
constexpr int kInkThreshold = 128;

int CharsetLength() {
  int n = 0;
  while (DebugOverlay::kCharset[n] != '\0')
    ++n;
  return n;
}

// One rasterization of the whole charset from a real font file.
// Cached process-wide on first use (function-local static,
// thread-safe); every query below reads the same atlas so glyphPixel
// and buildAtlas can never disagree. A missing/unusable font throws
// instead of degrading silently.
struct GlyphRaster {
  bool fromFont = false;
  std::string fontPath; // rasterized file, for diagnostics
  // Atlas bytes, row-major, glyphs packed left to right in kCharset
  // order: (count * kGlyphW) x kGlyphH, values 0 or 255 only.
  std::vector<unsigned char> atlas;
  // Pen advance per glyph in atlas pixels from the font xadvance
  // (proportional fonts included). buildVertices scales these by
  // kPixelScale.
  std::vector<int> advancePx;
};

bool LoadWholeFile(const char* path, std::vector<unsigned char>& out) {
  std::ifstream in(path, std::ios::binary);
  if (!in)
    return false;
  in.seekg(0, std::ios::end);
  const std::streamoff size = in.tellg();
  if (size <= 0)
    return false;
  in.seekg(0, std::ios::beg);
  out.resize(static_cast<size_t>(size));
  in.read(reinterpret_cast<char*>(out.data()), size);
  return static_cast<size_t>(in.gcount()) == out.size();
}

// Rasterizes every kCharset glyph from one font file into the fixed
// cell grid, following the stb sample pattern: baseline from VMetrics,
// per-glyph box from GetCodepointBitmapBox, blit at (x0, baseline+y0).
// Returns false when anything is off (bad file, missing glyph, cell
// overflow, threshold ate a glyph) so the caller tries the next font.
bool BlitFont(GlyphRaster& out, const std::vector<unsigned char>& bytes,
  const char* path) {
  stbtt_fontinfo font;
  if (stbtt_InitFont(&font, bytes.data(), 0) == 0)
    return false;
  const float scale = stbtt_ScaleForPixelHeight(&font, kFontPixelHeight);
  if (!(scale > 0.0f))
    return false;
  int ascent = 0;
  stbtt_GetFontVMetrics(&font, &ascent, nullptr, nullptr);
  const int baseline =
    static_cast<int>(static_cast<float>(ascent) * scale + 0.5f);
  if (baseline < 0 || baseline >= DebugOverlay::kGlyphH)
    return false;
  const int count = CharsetLength();
  const int atlasW = count * DebugOverlay::kGlyphW;
  std::vector<unsigned char> atlas(static_cast<size_t>(atlasW) *
    static_cast<size_t>(DebugOverlay::kGlyphH), 0);
  std::vector<int> advancePx(
    static_cast<size_t>(count), DebugOverlay::kGlyphW);
  for (int g = 0; g < count; ++g) {
    const char c = DebugOverlay::kCharset[g];
    const int cp = static_cast<unsigned char>(c);
    int advanceWidth = 0;
    stbtt_GetCodepointHMetrics(&font, cp, &advanceWidth, nullptr);
    int adv =
      static_cast<int>(static_cast<float>(advanceWidth) * scale + 0.5f);
    if (adv < 1)
      adv = 1;
    advancePx[static_cast<size_t>(g)] = adv;
    if (c == ' ')
      continue; // advance only
    if (stbtt_FindGlyphIndex(&font, cp) == 0)
      return false;
    int x0 = 0;
    int y0 = 0;
    int x1 = 0;
    int y1 = 0;
    stbtt_GetCodepointBitmapBox(&font, cp, scale, scale, &x0, &y0, &x1,
      &y1);
    const int bw = x1 - x0;
    const int bh = y1 - y0;
    if (bw <= 0 || bh <= 0)
      return false; // printable charset glyph with no bitmap: be strict
    if (bw > DebugOverlay::kGlyphW || bh > DebugOverlay::kGlyphH)
      return false; // cell overflow: try the next font
    std::vector<unsigned char> tmp(
      static_cast<size_t>(bw) * static_cast<size_t>(bh), 0);
    stbtt_MakeCodepointBitmap(&font, tmp.data(), bw, bh, bw, scale,
      scale, cp);
    bool ink = false;
    for (int yy = 0; yy < bh; ++yy) {
      const int dy = baseline + y0 + yy;
      // Clip to the cell: bitmap boxes carry font bearings that may
      // start a pixel outside (e.g. '$' top row, 'A' left column).
      // Rejecting the whole font over one clipped row is wrong.
      if (dy < 0 || dy >= DebugOverlay::kGlyphH)
        continue;
      for (int xx = 0; xx < bw; ++xx) {
        const int dx = x0 + xx;
        if (dx < 0 || dx >= DebugOverlay::kGlyphW)
          continue;
        if (tmp[static_cast<size_t>(yy) * static_cast<size_t>(bw) +
          static_cast<size_t>(xx)] >= kInkThreshold) {
          atlas[static_cast<size_t>(dy) * static_cast<size_t>(atlasW) +
            static_cast<size_t>(g * DebugOverlay::kGlyphW + dx)] = 255;
          ink = true;
        }
      }
    }
    if (!ink)
      return false; // threshold ate the glyph: try the next font
  }
  out.fromFont = true;
  out.fontPath = path;
  out.atlas = std::move(atlas);
  out.advancePx = std::move(advancePx);
  return true;
}

// Test seam for the font location: the app always uses the default
// repo-relative path, but test binaries run with a different working
// directory, so tests inject the absolute data-dir path. Must be set
// before first raster use (the atlas caches process-wide on first use).
std::string& FontPathOverride() {
  static std::string override;
  return override;
}

const GlyphRaster& GetRaster() {
  static const GlyphRaster raster = [] {
    GlyphRaster r;
    const std::string path = DebugOverlay::fontPath();
    std::vector<unsigned char> bytes;
    if (!LoadWholeFile(path.c_str(), bytes) || bytes.empty())
      throw std::runtime_error(
        "DebugOverlay: missing font file " + path);
    if (!BlitFont(r, bytes, path.c_str()))
      throw std::runtime_error(
        "DebugOverlay: cannot rasterize font file " + path);
    return r;
  }();
  return raster;
}

} // namespace

void DebugOverlay::setFontPath(const std::string& path) {
  FontPathOverride() = path;
}

std::string DebugOverlay::fontPath() {
  const std::string& override = FontPathOverride();
  return override.empty() ? kDefaultFontPath : override;
}

int DebugOverlay::glyphIndex(char c) {
  for (int i = 0; kCharset[i] != '\0'; ++i) {
    if (kCharset[i] == c)
      return i;
  }
  return -1;
}

bool DebugOverlay::glyphPixel(char c, int x, int y) {
  if (x < 0 || x >= kGlyphW || y < 0 || y >= kGlyphH)
    return false;
  const int idx = glyphIndex(c);
  if (idx < 0)
    return false;
  const GlyphRaster& raster = GetRaster();
  const int atlasW = glyphCount() * kGlyphW;
  return raster.atlas[static_cast<size_t>(y) *
    static_cast<size_t>(atlasW) +
    static_cast<size_t>(idx * kGlyphW + x)] != 0;
}

std::vector<unsigned char> DebugOverlay::buildAtlas(int& outW, int& outH) {
  const int count = glyphCount();
  outW = count * kGlyphW;
  outH = kGlyphH;
  return GetRaster().atlas; // copy of the cached single atlas
}

int DebugOverlay::glyphCount() { return CharsetLength(); }

DebugOverlay::DebugOverlay() = default;

DebugOverlay::~DebugOverlay() {
  // Only touches GL when the atlas was uploaded (needs a live context;
  // Window tears the overlay down while its context is still current).
  // A never-drawn overlay issues zero GL calls here.
  if (!glReady_)
    return;
  if (vbo_ != 0)
    glDeleteBuffers(1, &vbo_);
  if (vao_ != 0)
    glDeleteVertexArrays(1, &vao_);
  if (texture_ != 0)
    glDeleteTextures(1, &texture_);
  vbo_ = 0;
  vao_ = 0;
  texture_ = 0;
  glReady_ = false;
}

void DebugOverlay::setLines(const std::vector<std::string>& lines) {
  extraLines_ = lines;
}

void DebugOverlay::setVisible(bool visible) { visible_ = visible; }

void DebugOverlay::toggleVisible() { visible_ = !visible_; }

bool DebugOverlay::isVisible() const { return visible_; }

void DebugOverlay::update(double dtSeconds) {
  if (dtSeconds <= 0.0)
    return;
  const double ms = dtSeconds * 1000.0;
  if (!hasAvg_) {
    avgMs_ = ms;
    hasAvg_ = true;
  }
  else {
    avgMs_ += (ms - avgMs_) * 0.1;
  }
}

double DebugOverlay::fps() const {
  if (!hasAvg_ || avgMs_ <= 0.0)
    return 0.0;
  return 1000.0 / avgMs_;
}

double DebugOverlay::avgMs() const { return hasAvg_ ? avgMs_ : 0.0; }

void DebugOverlay::ensureGL() {
  if (glReady_)
    return;
  int w = 0;
  int h = 0;
  const std::vector<unsigned char> atlas = buildAtlas(w, h);
  atlasW_ = w;
  atlasH_ = h;
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glGenTextures(1, &texture_);
  glBindTexture(GL_TEXTURE_2D, texture_);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, w, h, 0, GL_RED,
    GL_UNSIGNED_BYTE, atlas.data());
  glBindTexture(GL_TEXTURE_2D, 0);
  glGenVertexArrays(1, &vao_);
  glGenBuffers(1, &vbo_);
  glReady_ = true;
}

void DebugOverlay::buildVertices(int fbWidth, int fbHeight,
  std::vector<float>& out) const {
  out.clear();
  char fpsLine[64];
  char msLine[64];
  std::snprintf(fpsLine, sizeof(fpsLine), "FPS %.0f", fps());
  std::snprintf(msLine, sizeof(msLine), "%.1f ms avg", avgMs());
  std::vector<std::string> lines;
  lines.emplace_back(fpsLine);
  lines.emplace_back(msLine);
  for (const std::string& extra : extraLines_)
    lines.push_back(extra);

  const float invW = 1.0f / static_cast<float>(fbWidth);
  const float invH = 1.0f / static_cast<float>(fbHeight);
  const float atlasInvW = 1.0f / static_cast<float>(atlasW_);
  const float atlasInvH = 1.0f / static_cast<float>(atlasH_);
  const int lineH = (kGlyphH + 1) * kPixelScale + kLineGapPx;

  auto emitQuad = [&](int px, int py, int pw, int ph, float u0, float v0,
      float u1, float v1) {
    // Pixel rect (top-left origin) to NDC triangles.
    const float x0 = static_cast<float>(px) * invW * 2.0f - 1.0f;
    const float x1 = static_cast<float>(px + pw) * invW * 2.0f - 1.0f;
    const float y0 = 1.0f - static_cast<float>(py + ph) * invH * 2.0f;
    const float y1 = 1.0f - static_cast<float>(py) * invH * 2.0f;
    // No v-flip: glTexImage2D places buffer row 0 at texture bottom
    // (v=0), and buffer row 0 is the glyph top - exactly where the
    // top-down screen rows expect it. So glyph row y spans v0..v1
    // directly: pixel bottom edge (higher row boundary) takes v1,
    // pixel top edge takes v0. (A flip here mirrors glyphs and
    // collapses most rows to blank - debugged the hard way.)
    const float va = v1;
    const float vb = v0;
    const float quad[6][4] = {
      {x0, y0, u0, va}, {x1, y0, u1, va}, {x1, y1, u1, vb},
      {x0, y0, u0, va}, {x1, y1, u1, vb}, {x0, y1, u0, vb},
    };
    for (const auto& v : quad) {
      out.push_back(v[0]);
      out.push_back(v[1]);
      out.push_back(v[2]);
      out.push_back(v[3]);
    }
  };

  for (size_t li = 0; li < lines.size(); ++li) {
    const std::string& line = lines[li];
    const int baseY = kMarginPx + static_cast<int>(li) * lineH;
    int penX = kMarginPx;
    for (size_t ci = 0; ci < line.size(); ++ci) {
      const int idx = glyphIndex(line[ci]);
      if (idx < 0) {
        // unsupported chars render blank (advance only)
        penX += (kGlyphW + 1) * kPixelScale;
        continue;
      }
      bool blank = true;
      for (int y = 0; y < kGlyphH && blank; ++y) {
        for (int x = 0; x < kGlyphW && blank; ++x) {
          if (glyphPixel(line[ci], x, y))
            blank = false;
        }
      }
      if (!blank) {
        for (int y = 0; y < kGlyphH; ++y) {
          for (int x = 0; x < kGlyphW; ++x) {
            if (!glyphPixel(line[ci], x, y))
              continue;
            const float u0 =
              static_cast<float>(idx * kGlyphW + x) * atlasInvW;
            const float v0 = static_cast<float>(y) * atlasInvH;
            const float u1 =
              static_cast<float>(idx * kGlyphW + x + 1) * atlasInvW;
            const float v1 = static_cast<float>(y + 1) * atlasInvH;
            emitQuad(penX + x * kPixelScale, baseY + y * kPixelScale,
              kPixelScale, kPixelScale, u0, v0, u1, v1);
          }
        }
      }
      // Proportional pen: per-glyph xadvance from the raster cache.
      // Space (and any blank cell) advances with no quad.
      penX += GetRaster().advancePx[static_cast<size_t>(idx)] *
        kPixelScale;
    }
  }
}

void DebugOverlay::draw(int fbWidth, int fbHeight) {
  // Hidden means hidden: zero GL calls so the post chain is untouched.
  if (!visible_)
    return;
  if (fbWidth <= 0 || fbHeight <= 0)
    return;
  ensureGL();

  Shader* shader = nullptr;
  try {
    shader = ShaderLibrary::getShader("ss_debugtext");
  }
  catch (...) {
    return;
  }

  std::vector<float> verts;
  buildVertices(fbWidth, fbHeight, verts);
  if (verts.empty())
    return;

  // Save the state the overlay touches; the post chain behind us must
  // read back exactly what it left.
  GLint prevViewport[4] = {0, 0, 0, 0};
  glGetIntegerv(GL_VIEWPORT, prevViewport);
  GLboolean blendOn = glIsEnabled(GL_BLEND);
  GLboolean depthOn = glIsEnabled(GL_DEPTH_TEST);
  GLboolean cullOn = glIsEnabled(GL_CULL_FACE);
  GLint prevSrc = GL_ONE;
  GLint prevDst = GL_ZERO;
  glGetIntegerv(GL_BLEND_SRC_RGB, &prevSrc);
  glGetIntegerv(GL_BLEND_DST_RGB, &prevDst);
  GLint prevProgram = 0;
  glGetIntegerv(GL_CURRENT_PROGRAM, &prevProgram);
  GLint prevActive = GL_TEXTURE0;
  glGetIntegerv(GL_ACTIVE_TEXTURE, &prevActive);
  GLint prevTex = 0;
  glGetIntegerv(GL_TEXTURE_BINDING_2D, &prevTex);
  GLint prevVao = 0;
  glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &prevVao);
  GLint prevVbo = 0;
  glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &prevVbo);

  glViewport(0, 0, fbWidth, fbHeight);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_CULL_FACE);

  shader->bind();
  const unsigned int posLoc = shader->attribute("position");
  const unsigned int uvLoc = shader->attribute("texcoord");
  glUniform1i(shader->uniform("texAtlas"), 0);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, texture_);

  glBindVertexArray(vao_);
  glBindBuffer(GL_ARRAY_BUFFER, vbo_);
  glBufferData(GL_ARRAY_BUFFER,
    static_cast<GLsizeiptr>(verts.size() * sizeof(float)), verts.data(),
    GL_DYNAMIC_DRAW);
  glEnableVertexAttribArray(posLoc);
  glVertexAttribPointer(posLoc, 2, GL_FLOAT, GL_FALSE,
    4 * sizeof(float), reinterpret_cast<void*>(0));
  glEnableVertexAttribArray(uvLoc);
  glVertexAttribPointer(uvLoc, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
    reinterpret_cast<void*>(2 * sizeof(float)));

  glDrawArrays(GL_TRIANGLES, 0,
    static_cast<GLsizei>(verts.size() / 4));

  glDisableVertexAttribArray(posLoc);
  glDisableVertexAttribArray(uvLoc);
  glBindVertexArray(static_cast<GLuint>(prevVao));
  glBindBuffer(GL_ARRAY_BUFFER, static_cast<GLuint>(prevVbo));
  glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(prevTex));
  glActiveTexture(static_cast<GLenum>(prevActive));
  glUseProgram(static_cast<GLuint>(prevProgram));
  if (cullOn == GL_TRUE)
    glEnable(GL_CULL_FACE);
  else
    glDisable(GL_CULL_FACE);
  if (depthOn == GL_TRUE)
    glEnable(GL_DEPTH_TEST);
  else
    glDisable(GL_DEPTH_TEST);
  if (blendOn == GL_TRUE)
    glEnable(GL_BLEND);
  else
    glDisable(GL_BLEND);
  glBlendFunc(static_cast<GLenum>(prevSrc), static_cast<GLenum>(prevDst));
  glViewport(prevViewport[0], prevViewport[1], prevViewport[2],
    prevViewport[3]);
}
