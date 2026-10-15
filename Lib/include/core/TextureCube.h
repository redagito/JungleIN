#pragma once

#include <string>
#include <vector>

#include "core/Texture.h"
#include "core/Image.h"

class TextureCube {
public:
  TextureCube(std::string texDir, std::string ext, unsigned int _index = 0);
  ~TextureCube();

  void init();
  void bind();

  bool load(std::string texDir, std::string ext);

  // Needs to be public to be accessed by GL calls
  unsigned int gluid = 0;
  unsigned int glunit = 0;
  unsigned int index = 0;

private:
  std::vector<Image> images;
};