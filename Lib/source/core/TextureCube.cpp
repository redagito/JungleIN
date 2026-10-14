#include "core/TextureCube.h"

#include <stdexcept>

#include <fmtlog/fmtlog.h>

/*!
 * Pathes order : px, nx, py, ny, pz, nz
 */
TextureCube::TextureCube(std::string texDir, std::string ext, unsigned int _index,
	unsigned int _size) {
	gluid = 0;
	glunit = Texture::unitFromIndex(_index);

	index = _index;
	size = _size;
	load(texDir, ext);

}

TextureCube::~TextureCube() {
	glDeleteTextures(1, &gluid);
	images.clear();
}

bool TextureCube::load(std::string texDir, std::string ext) {
	images.push_back(Image(texDir + "px" + ext));
	images.push_back(Image(texDir + "nx" + ext));
	images.push_back(Image(texDir + "py" + ext));
	images.push_back(Image(texDir + "ny" + ext));
	images.push_back(Image(texDir + "pz" + ext));
	images.push_back(Image(texDir + "nz" + ext));
	// GL uploads every face with one fixed size: mismatched faces would
	// silently corrupt the cube, so fail fast here instead.
	const unsigned int width = images.front().getWidth();
	const unsigned int height = images.front().getHeight();
	for (const Image& image : images) {
		if (image.getWidth() != width || image.getHeight() != height)
			throw std::runtime_error("Cubemap face size mismatch in " + texDir);
	}
	return true;
}

void TextureCube::init() {
	if (images.size() != 6) {
		logi("Incomplete cube map textures.");
		return;
	}

	glActiveTexture(glunit);

	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

	glGenTextures(1, &gluid);

	glBindTexture(GL_TEXTURE_CUBE_MAP, gluid);

	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, GL_RGBA, size, size, 0,
		GL_RGBA, GL_UNSIGNED_BYTE, images.at(0).getData());
	glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 0, GL_RGBA, size, size, 0,
		GL_RGBA, GL_UNSIGNED_BYTE, images.at(1).getData());
	glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 0, GL_RGBA, size, size, 0,
		GL_RGBA, GL_UNSIGNED_BYTE, images.at(2).getData());
	glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 0, GL_RGBA, size, size, 0,
		GL_RGBA, GL_UNSIGNED_BYTE, images.at(3).getData());
	glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 0, GL_RGBA, size, size, 0,
		GL_RGBA, GL_UNSIGNED_BYTE, images.at(4).getData());
	glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 0, GL_RGBA, size, size, 0,
		GL_RGBA, GL_UNSIGNED_BYTE, images.at(5).getData());

}

void TextureCube::bind() {
	glActiveTexture(glunit);
	glBindTexture(GL_TEXTURE_CUBE_MAP, gluid);

}
