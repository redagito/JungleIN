#include "framebuffer.h"

#include <iostream>
#include <flextGL.h>

#include "helpers/utils.h"
#include "render/Debug.h"

const int FrameBuffer::RT_SIZE_DEFAULT = 1024;

FrameBuffer::FrameBuffer(bool storeDepth) {
  uid = 0;
  hasDepth = storeDepth;
  texturesNames = std::vector<std::string>();
  textures = std::vector<Texture *>();
  width = FrameBuffer::RT_SIZE_DEFAULT;
  height = FrameBuffer::RT_SIZE_DEFAULT;
}

FrameBuffer::FrameBuffer(std::vector<std::string> targets, unsigned int _width,
                         unsigned int _height, bool storeDepth) {
  uid = 0;
  hasDepth = storeDepth;
  texturesNames = targets;
  textures = std::vector<Texture *>();
  width = _width;
  height = _height;
}

FrameBuffer::~FrameBuffer() {
  for (Texture *tex : textures) {
    delete tex;
  }
  textures.clear();

  delete drawBuffers;
}

void FrameBuffer::clear() {
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void FrameBuffer::init(int textureUnitOffset, bool linearFilter) {
  glGenFramebuffers(1, &uid);
  PRINT_GL_ERROR();
  glBindFramebuffer(GL_FRAMEBUFFER, uid);
  PRINT_GL_ERROR();

  int bcount = texturesNames.size(); //+ (hasDepth ? 1 : 0);
  if (bcount <= 0) {
    std::cout << "Empty framebuffer." << std::endl;
    return;
  } else {
    size = bcount;
  }

  drawBuffers = new unsigned int[bcount];
  bcount = 0;

  if (hasDepth) {
    glGenRenderbuffers(1, &depthBuffer);
    PRINT_GL_ERROR();
    glBindRenderbuffer(GL_RENDERBUFFER, depthBuffer);
    PRINT_GL_ERROR();
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height);
    PRINT_GL_ERROR();
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                              GL_RENDERBUFFER, depthBuffer);
    PRINT_GL_ERROR();
  }

  if (texturesNames.size() > 0) {
    Texture::resetUnit(textureUnitOffset);
    for (size_t i = 0; i < texturesNames.size(); ++i) {
      Texture *tex = Texture::newFromNextUnit();
      tex->setupForFramebuffer(width, height, GL_RGB32F);
      if (linearFilter)
        tex->setFilters(GL_LINEAR, GL_LINEAR);
      tex->init();
      tex->bind();

      unsigned int attachment = getAttachementFromIndex(i);

      glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, GL_TEXTURE_2D,
                             tex->gluid, 0);
      PRINT_GL_ERROR();

      textures.push_back(tex);
      drawBuffers[bcount++] = attachment;
    }
  }
  PRINT_GL_ERROR();

  glDrawBuffers(size, drawBuffers);
  PRINT_GL_ERROR();
  unbind();
  PRINT_GL_ERROR();
}

void FrameBuffer::bind() {
  glBindFramebuffer(GL_FRAMEBUFFER, uid);
  glDrawBuffers(size, drawBuffers);
}

void FrameBuffer::unbind() { glBindFramebuffer(GL_FRAMEBUFFER, 0); }

void FrameBuffer::resize(unsigned int _width, unsigned int _height) {
  width = _width;
  height = _height;

  if (hasDepth) {
    glBindRenderbuffer(GL_RENDERBUFFER, depthBuffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height);
  }

  for (Texture *tex : textures) {
    tex->resize(width, height);
  }
}

void FrameBuffer::resizeViewport() { glViewport(0, 0, width, height); }

void FrameBuffer::bindAndTransmitTextures(Shader *shader) {
  for (int i = 0; i < texturesNames.size(); ++i) {
    Texture *tex = textures.at(i);
    tex->bind();
    shader->transmitUniform(texturesNames.at(i), tex);
  }
}

unsigned int FrameBuffer::getAttachementFromIndex(unsigned int index) {
  switch (index) {
  case 1:
    return GL_COLOR_ATTACHMENT1;
  case 2:
    return GL_COLOR_ATTACHMENT2;
  case 3:
    return GL_COLOR_ATTACHMENT3;
  case 4:
    return GL_COLOR_ATTACHMENT4;
  case 5:
    return GL_COLOR_ATTACHMENT5;
  case 6:
    return GL_COLOR_ATTACHMENT6;
  case 7:
    return GL_COLOR_ATTACHMENT7;
  case 8:
    return GL_COLOR_ATTACHMENT8;
  case 9:
    return GL_COLOR_ATTACHMENT9;
  default:
    return GL_COLOR_ATTACHMENT0;
  }
}

Texture *FrameBuffer::getTexture(std::string name) {
  return textures.at(indexOf(texturesNames, name));
}

unsigned int FrameBuffer::getWidth() { return width; }

unsigned int FrameBuffer::getHeight() { return height; }
