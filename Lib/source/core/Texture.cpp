#include "core/Texture.h"

#include <stb_image.h>

namespace {
// Sized and unsized depth internal formats all upload as float depth;
// anything else stays an unsigned-byte color texture.
bool IsDepthFormat(unsigned int format) {
	return format == GL_DEPTH_COMPONENT ||
		format == GL_DEPTH_COMPONENT16 ||
		format == GL_DEPTH_COMPONENT24 ||
		format == GL_DEPTH_COMPONENT32 ||
		format == GL_DEPTH_COMPONENT32F;
}
} // namespace

//const unsigned int Texture::LINEAR = GL_LINEAR;
//const unsigned int Texture::NEAREST = GL_NEAREST;
//const unsigned int Texture::MIPMAP = GL_LINEAR_MIPMAP_LINEAR;

Texture::Texture() {
	height = 0;
	width = 0;
	hasImage = false;
	format = GL_RGBA;
	gluid = 0;
	glunit = 0;
	index = 0;
	minFilter = GL_LINEAR;
	magFilter = GL_LINEAR;
	globalFormat = GL_RGBA;
	pixelType = GL_UNSIGNED_BYTE;
	shadowCompare = false;
}

Texture::Texture(std::string path, unsigned int _index) {
	load(path);
	gluid = 0;
	glunit = unitFromIndex(_index);
	index = _index;
	minFilter = GL_LINEAR;
	magFilter = GL_LINEAR;
	globalFormat = GL_RGBA;
	pixelType = GL_UNSIGNED_BYTE;
	shadowCompare = false;
}

// NOTE: render-target buffers stay 8-bit GL_RGBA by design for now;
// float HDR buffers (e.g. RGBA16F) are planned for a later pass.
Texture::Texture(unsigned int _index) {
	height = 0;
	width = 0;
	hasImage = false;
	format = GL_RGBA;
	index = _index;
	glunit = unitFromIndex(_index);
	gluid = 0;
	hasImage = false;
	minFilter = GL_LINEAR;
	magFilter = GL_LINEAR;
	globalFormat = GL_RGBA;
	pixelType = GL_UNSIGNED_BYTE;
	shadowCompare = false;
}

Texture::~Texture() { glDeleteTextures(1, &gluid); }

bool Texture::load(std::string path)
{
	// TODO Duplicate code, see Image class
	// Flip for opengl textures
	stbi_set_flip_vertically_on_load(true);
	int channels = 0;
	int x = 0;
	int y = 0;

	// Load as RGBA
	auto data = stbi_load(path.c_str(), &x, &y, &channels, STBI_rgb_alpha);
	if (data == nullptr)
	{
		throw std::runtime_error("Failed to load image");
	}
	image.assign(data, data + x * y * 4);
	width = x;
	height = y;
	stbi_image_free(data);

	format = GL_RGBA;
	hasImage = true;
	return true;
}

void Texture::setupForFramebuffer(unsigned int _width, unsigned int _height,
	unsigned int _format,
	unsigned int _globalFormat) {
	width = _width;
	height = _height;
	format = _format;
	globalFormat = _globalFormat;
	pixelType = IsDepthFormat(format) ? GL_FLOAT : GL_UNSIGNED_BYTE;
	shadowCompare = false;
	minFilter = GL_LINEAR;
	magFilter = GL_LINEAR;
}

void Texture::setFilters(GLint min, GLint mag) {
	minFilter = min;
	magFilter = mag;
}

void Texture::setupForDepthShadow(unsigned int _width, unsigned int _height) {
	width = _width;
	height = _height;
	format = GL_DEPTH_COMPONENT24;
	globalFormat = GL_DEPTH_COMPONENT;
	pixelType = GL_FLOAT;
	minFilter = GL_LINEAR;
	magFilter = GL_LINEAR;
	shadowCompare = true;
}

void Texture::enableShadowCompare() {
	shadowCompare = true;
	if (gluid != 0) {
		bind();
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE,
			GL_COMPARE_REF_TO_TEXTURE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
	}
}

void Texture::init() {
	
	glActiveTexture(glunit);
	
	glGenTextures(1, &gluid);
	
	glBindTexture(GL_TEXTURE_2D, gluid);
	

	if (hasImage) {
		glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, globalFormat,
			pixelType, image.data());
		

		if (minFilter == GL_LINEAR_MIPMAP_LINEAR) {
			glGenerateMipmap(GL_TEXTURE_2D);
			
		}

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		

		if (Utils::USE_ANISO) {
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY,
				Utils::MAX_ANISO);
		}
	}
	else {
		glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, globalFormat,
			pixelType, NULL);
		
		if (IsDepthFormat(format)) {
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
			
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
			
			glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

			if (shadowCompare) {
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE,
					GL_COMPARE_REF_TO_TEXTURE);

				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC,
					GL_LEQUAL);
			}
			
		}
		else {
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
			
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
			
			glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColorB);
			
		}
	}
	
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);
	
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilter);
	
}

void Texture::bind() {
	glActiveTexture(glunit);
	glBindTexture(GL_TEXTURE_2D, gluid);

	if (Texture::needsUpdate)
		update();
}

void Texture::update() {
	if (Utils::USE_ANISO) {
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY,
			Utils::MAX_ANISO);
	}
	else {
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY, 1.0f);
	}
}

void Texture::resize(unsigned int _width, unsigned int _height) {
	width = _width;
	height = _height;

	if (!hasImage) {
		bind();
		glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, globalFormat,
			pixelType, NULL);
	}
}

int Texture::getHeight() { return height; }

int Texture::getWidth() { return width; }

/*!
 * @static
 */

bool Texture::needsUpdate = false;

unsigned int Texture::unitCount = 0;

float Texture::borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
float Texture::borderColorB[] = { 0.0f, 0.0f, 0.0f, 0.0f };

void Texture::resetUnit(int textureUnitOffset) {
	unitCount = textureUnitOffset;
}

Texture Texture::fromNextUnit() { return Texture(unitCount++); }

Texture* Texture::newFromNextUnit() { return new Texture(unitCount++); }

unsigned int Texture::unitFromIndex(unsigned int index) {
	return GL_TEXTURE0 + index;
}
