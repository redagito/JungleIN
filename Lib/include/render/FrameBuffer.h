#pragma once

#include <vector>
#include <string>

#include <glad/gl.h>

#include "core/Texture.h"
#include "render/Shader.h"

/*!
 * FrameBuffer
 * Stores an FBO and its corresponding textures.
 */
class FrameBuffer {
public:
  FrameBuffer(bool storeDepth = false);
  FrameBuffer(std::vector<std::string> textures, unsigned int _width = RT_SIZE_DEFAULT,
              unsigned int _height = RT_SIZE_DEFAULT, bool storeDepth = false);
  ~FrameBuffer();

  void clear();
  void init(int textureUnitOffset = 0, bool linearFilter = false);
  void bind();
  void bindAndViewport();
  void unbind();
  void resize(unsigned int _width, unsigned int _height);
  void resizeViewport();
  void assertComplete() const;

  void bindAndTransmitTextures(Shader *shader);

  unsigned int getAttachementFromIndex(unsigned int index);

  Texture *getTexture(std::string name);
  unsigned int getWidth();
  unsigned int getHeight();

  static const int RT_SIZE_DEFAULT;

protected:
  GLuint uid = 0;

  unsigned int width = 0;
  unsigned int height = 0;

  bool hasDepth = false;
  GLuint depthBuffer = 0;

  std::vector<std::string> texturesNames;
  std::vector<Texture *> textures;

  unsigned int size = 0;
  GLuint *drawBuffers = nullptr;
};