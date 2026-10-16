#pragma once

#include "render/FrameBuffer.h"

class ShadowBuffer : public FrameBuffer {
public:
	ShadowBuffer(std::string texname, int width, int height);
	~ShadowBuffer() override = default;
	ShadowBuffer(const ShadowBuffer&) = delete;
	ShadowBuffer& operator=(const ShadowBuffer&) = delete;
	void init(unsigned int textureUnitOffset = 0);
};