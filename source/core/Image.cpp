#include "Image.h"

#include <lodepng.h>

Image::Image() {}

Image::Image(const std::string &file) { load(file); }

bool Image::load(const std::string &file) {
  return lodepng::decode(m_data, m_width, m_height, file) == 0;
}

const unsigned char *Image::getData() const { return m_data.data(); }

unsigned int Image::getWidth() const { return m_width; }

unsigned int Image::getHeight() const { return m_height; }

Image::RGBA Image::getPixel(unsigned x, unsigned y) const {
  Image::RGBA rgba;
  rgba.r = m_data.at(y * m_width + x * 4);
  rgba.g = m_data.at(y * m_width + x * 4 + 1);
  rgba.b = m_data.at(y * m_width + x * 4 + 2);
  rgba.a = m_data.at(y * m_width + x * 4 + 3);
  return rgba;
}
