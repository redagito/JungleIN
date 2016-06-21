#include "Image.h"

#include <lodepng.h>

Image::Image() { }

Image::Image(const std::string& file) {
	load(file);
}

bool Image::load(const std::string& file) {
	return lodepng::decode(m_data, m_width, m_height, file) == 0;
}

const unsigned char* Image::getData() const {
	return m_data.data();
}

unsigned int Image::getWidth() const {
	return m_width;
}

unsigned int Image::getHeight() const {
	return m_height;
}