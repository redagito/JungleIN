#include "Image.h"

#include <fmtlog/fmtlog.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <stdexcept>

Image::Image(const std::string& file) 
{
	if (!load(file))
		throw std::runtime_error("Failed to load image from file: " + file);
}

bool Image::load(const std::string& file)
{
	// Flip for opengl textures
	stbi_set_flip_vertically_on_load(true);
	int channels = 0;
	int x = 0;
	int y = 0;

	// Load as RGBA
	auto data = stbi_load(file.c_str(), &x, &y, &channels, STBI_rgb_alpha);
	if (data == nullptr)
	{
		logw("Failed to load image {}", file);
		return false;
	}
	m_data.assign(data, data + x * y * 4);
	m_width = x;
	m_height = y;

	stbi_image_free(data);
	return true;
}

const unsigned char* Image::getData() const { return m_data.data(); }

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
