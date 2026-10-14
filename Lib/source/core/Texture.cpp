#include "core/Texture.h"

#include <stb_image.h>

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
}

Texture::Texture(std::string path, unsigned int _index) {
	load(path);
	gluid = 0;
	glunit = unitFromIndex(_index);
	index = _index;
	minFilter = GL_LINEAR;
	magFilter = GL_LINEAR;
	globalFormat = GL_RGBA;
}

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
	minFilter = GL_LINEAR;
	magFilter = GL_LINEAR;
}

void Texture::setFilters(GLint min, GLint mag) {
	minFilter = min;
	magFilter = mag;
}

void Texture::init() {
	
	glActiveTexture(glunit);
	
	glGenTextures(1, &gluid);
	
	glBindTexture(GL_TEXTURE_2D, gluid);
	

	if (hasImage) {
		glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, globalFormat,
			GL_UNSIGNED_BYTE, image.data());
		

		if (minFilter == GL_LINEAR_MIPMAP_LINEAR) {
			glGenerateMipmap(GL_TEXTURE_2D);
			
		}

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		

		if (Utils::USE_ANISO) {
			//glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT,
			//	Utils::MAX_ANISO);
		}
	}
	else {
		glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, globalFormat,
			GL_UNSIGNED_BYTE, NULL);
		
		if (format == GL_DEPTH_COMPONENT) {
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
			
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
			
			glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
			
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
		//glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT,
		//	Utils::MAX_ANISO);
	}
	else {
		//glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 0);
	}
}

void Texture::resize(unsigned int _width, unsigned int _height) {
	width = _width;
	height = _height;

	if (!hasImage) {
		bind();
		glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, globalFormat,
			GL_UNSIGNED_BYTE, NULL);
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
