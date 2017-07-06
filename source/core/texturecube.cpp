#include "texturecube.h"

#include <iostream>

#include "render/Debug.h"

/*!
 * Pathes order : px, nx, py, ny, pz, nz
 */
TextureCube::TextureCube(std::string texDir, std::string ext, unsigned int _index,
                         unsigned int _size) {
  gluid = 0;
  glunit = Texture::unitFromIndex(_index);
  PRINT_GL_ERROR();
  index = _index;
  size = _size;
  load(texDir);
  PRINT_GL_ERROR();
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
	return true;
}

void TextureCube::init() {
  if (images.size() != 6) {
    std::cout << "Incomplete cube map textures." << std::endl;
    return;
  }

  PRINT_GL_ERROR();
  glActiveTexture(glunit);
  PRINT_GL_ERROR();
  glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
  PRINT_GL_ERROR();
  glGenTextures(1, &gluid);
  PRINT_GL_ERROR();

  glBindTexture(GL_TEXTURE_CUBE_MAP, gluid);
  PRINT_GL_ERROR();

  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
  PRINT_GL_ERROR();

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
  PRINT_GL_ERROR();
}

void TextureCube::bind() {
  glActiveTexture(glunit);
  glBindTexture(GL_TEXTURE_CUBE_MAP, gluid);
  PRINT_GL_ERROR();
}
