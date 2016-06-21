#ifndef TEXTURE_H
#define TEXTURE_H

#include <string>
#include <vector>

#include <flextGL.h>

#include "../helpers/utils.h"

/*!
 * Texture
 * Structure de donn�e permettant de stocker l'index, le GL_TEXTUREX et l'uid
 * d'une texture OpenGL.
 * Peut aussi contenir une QImage si charg�e en externe.
 */
class Texture {
public:
  Texture();
  Texture(std::string path, unsigned int _index = 0);
  Texture(unsigned int _index);

  ~Texture();

  void init();
  void bind();
  void update();
  void setupForFramebuffer(unsigned int _width, unsigned int _height,
                           unsigned int _format,
                           unsigned int _globalFormat = GL_RGBA);
  bool load(std::string path);
  void resize(unsigned int _width, unsigned int _height);

  void setFilters(unsigned int min, unsigned int mag);

  static void resetUnit(int textureUnitOffset = 0);
  static Texture fromNextUnit();
  static Texture *newFromNextUnit();
  static unsigned int unitFromIndex(unsigned int index);

  int getHeight();
  int getWidth();

  // Needs to be public to be accessed by GL calls
  unsigned int gluid;
  unsigned int glunit;
  unsigned int index;

  static bool needsUpdate;

  static const unsigned int LINEAR;
  static const unsigned int NEAREST;
  static const unsigned int MIPMAP;

private:
  static unsigned int unitCount;
  static float borderColor[];
  static float borderColorB[];

  std::vector<unsigned char> image;
  bool hasImage;
  unsigned int height;
  unsigned int width;
  unsigned int minFilter;
  unsigned int magFilter;
  unsigned int format;
  unsigned int globalFormat;
};

#endif // TEXTURE_H
