#pragma once

#include "framebuffer.h"

class ShadowBuffer : public FrameBuffer {
public:
	ShadowBuffer(std::string texname, int width, int height);
	void init(unsigned int textureUnitOffset = 0);
};