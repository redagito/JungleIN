#ifndef TEXTURECUBE_H
#define TEXTURECUBE_H

#include <string>
#include <vector>

#include "texture.h"
#include "core/Image.h"

class TextureCube {
public:
  TextureCube(std::string texDir, std::string ext = ".png", unsigned int _index = 0,
              unsigned int _size = 1024);
  ~TextureCube();

  void init();
  void bind();

  bool load(std::string texDir, std::string ext = ".png");

  // Needs to be public to be accessed by GL calls
  unsigned int gluid;
  unsigned int glunit;
  unsigned int index;

private:
  std::vector<Image> images;
  unsigned int size;
};

#endif // TEXTURECUBE_H
