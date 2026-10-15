#pragma once

#include <string>
#include <vector>

#include "core/ImageRGBA.h"

class Image {
public:
  using RGBA = ImageRGBA;

  Image() = default;
  // flipVertically preserves the historical QImage top-left origin when
  // false (used for data maps like heightmaps); GL textures keep true.
  Image(const std::string &file, bool flipVertically = true);
  bool load(const std::string &path, bool flipVertically = true);

  const unsigned char *getData() const;

  unsigned int getWidth() const;
  unsigned int getHeight() const;

  RGBA getPixel(unsigned x, unsigned y) const;

private:
  unsigned int m_width = 0;
  unsigned int m_height = 0;
  std::vector<unsigned char> m_data;
};
