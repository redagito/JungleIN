#include "render/ShadowBuffer.h"

#include <stdexcept>
#include <string>

ShadowBuffer::ShadowBuffer(std::string texname, int width, int height) {
	this->width = width;
	this->height = height;
	uid = 0;
	texturesNames = std::vector<std::string>();
	texturesNames.push_back(texname);
	textures = std::vector<Texture*>();
}

void ShadowBuffer::init(unsigned int textureUnitOffset) 
{
	glGenFramebuffers(1, &uid);
	glBindFramebuffer(GL_FRAMEBUFFER, uid);

	size = 1;

	drawBuffers = new unsigned int[size];

	Texture::resetUnit(textureUnitOffset);
	Texture* tex = Texture::newFromNextUnit();
	tex->setupForFramebuffer(width, height, GL_DEPTH_COMPONENT,
		GL_DEPTH_COMPONENT);
	tex->setFilters(GL_LINEAR, GL_LINEAR);
	tex->init();
	tex->bind();
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D,
		tex->gluid, 0);

	textures.push_back(tex);

	drawBuffers[0] = GL_NONE;

	glDrawBuffers(size, drawBuffers);

	GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (status != GL_FRAMEBUFFER_COMPLETE)
		throw std::runtime_error("Shadow framebuffer incomplete: " +
			std::to_string(status));

	unbind();
}
