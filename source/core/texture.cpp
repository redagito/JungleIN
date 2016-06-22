#include "texture.h"

#include <iostream>

#include <lodepng.h>

#include "render/Debug.h"

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

bool Texture::load(std::string path) {
	if (lodepng::decode(image, width, height, path) != 0) {
		std::cout << "Failed to load " << path << std::endl;
		return false;
	}
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
	PRINT_GL_ERROR();
  glActiveTexture(glunit);
  PRINT_GL_ERROR();
  glGenTextures(1, &gluid);
  PRINT_GL_ERROR();
  glBindTexture(GL_TEXTURE_2D, gluid);
  PRINT_GL_ERROR();

  if (hasImage) {
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, globalFormat,
                 GL_UNSIGNED_BYTE, image.data());
	PRINT_GL_ERROR();

	if (minFilter == GL_LINEAR_MIPMAP_LINEAR) {
		glGenerateMipmap(GL_TEXTURE_2D);
		PRINT_GL_ERROR();
	}

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	PRINT_GL_ERROR();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	PRINT_GL_ERROR();

	if (Utils::USE_ANISO) {
		//glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT,
		//	Utils::MAX_ANISO);
	}
  } else {
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, globalFormat,
                 GL_UNSIGNED_BYTE, NULL);
	PRINT_GL_ERROR();
    if (format == GL_DEPTH_COMPONENT) {
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	  PRINT_GL_ERROR();
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	  PRINT_GL_ERROR();
      glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
	  PRINT_GL_ERROR();
    } else {
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	  PRINT_GL_ERROR();
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	  PRINT_GL_ERROR();
      glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColorB);
	  PRINT_GL_ERROR();
    }
  }
  PRINT_GL_ERROR();
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);
  PRINT_GL_ERROR();
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilter);
  PRINT_GL_ERROR();
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

float Texture::borderColor[] = {1.0f, 1.0f, 1.0f, 1.0f};
float Texture::borderColorB[] = {0.0f, 0.0f, 0.0f, 0.0f};

void Texture::resetUnit(int textureUnitOffset) {
  unitCount = textureUnitOffset;
}

Texture Texture::fromNextUnit() { return Texture(unitCount++); }

Texture *Texture::newFromNextUnit() { return new Texture(unitCount++); }

unsigned int Texture::unitFromIndex(unsigned int index) {
	return GL_TEXTURE0 + index;
}
