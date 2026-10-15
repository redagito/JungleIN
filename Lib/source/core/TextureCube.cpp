#include "core/TextureCube.h"

#include <stdexcept>

#include "helpers/Logger.h"

/*!
 * Pathes order : px, nx, py, ny, pz, nz
 */
TextureCube::TextureCube(std::string texDir, std::string ext, unsigned int _index) {
	gluid = 0;
	glunit = Texture::unitFromIndex(_index);

	index = _index;
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
	// GL requires square faces uploaded at their actual size: inconsistent
	// sets would silently corrupt the cube, so fail fast here instead.
	const unsigned int width = images.front().getWidth();
	const unsigned int height = images.front().getHeight();
	if (width != height) {
		images.clear();
		throw std::runtime_error("Cubemap faces are not square in " + texDir);
	}
	for (const Image& image : images) {
		if (image.getWidth() != width || image.getHeight() != height) {
			images.clear();
			throw std::runtime_error("Cubemap face size mismatch in " + texDir);
		}
	}
	return true;
}

void TextureCube::init() {
	if (images.size() != 6)
		throw std::runtime_error("Incomplete cube map textures.");

	glActiveTexture(glunit);

	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

	glGenTextures(1, &gluid);

	glBindTexture(GL_TEXTURE_CUBE_MAP, gluid);

	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	const unsigned int faceTargets[6] = {
		GL_TEXTURE_CUBE_MAP_POSITIVE_X, GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
		GL_TEXTURE_CUBE_MAP_POSITIVE_Y, GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
		GL_TEXTURE_CUBE_MAP_POSITIVE_Z, GL_TEXTURE_CUBE_MAP_NEGATIVE_Z};
	for (int face = 0; face < 6; ++face) {
		const Image& image = images.at(static_cast<size_t>(face));
		glTexImage2D(faceTargets[face], 0, GL_RGBA,
			static_cast<int>(image.getWidth()),
			static_cast<int>(image.getHeight()), 0, GL_RGBA,
			GL_UNSIGNED_BYTE, image.getData());
	}

}

void TextureCube::bind() {
	glActiveTexture(glunit);
	glBindTexture(GL_TEXTURE_CUBE_MAP, gluid);

}
