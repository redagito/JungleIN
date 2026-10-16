#include "materials/MaterialGround.h"

MaterialGround::MaterialGround(std::unique_ptr<Texture> moss,
	                         std::unique_ptr<Texture> earth,
	                         std::unique_ptr<Texture> shatter,
	                         std::unique_ptr<Texture> mossNormal,
	                         std::unique_ptr<Texture> earthNormal,
	                         std::unique_ptr<Texture> shatterNormal)
	: Material("rt_ground_n"), moss(std::move(moss)), earth(std::move(earth)),
	  shatter(std::move(shatter)), mossNormal(std::move(mossNormal)),
	  earthNormal(std::move(earthNormal)),
	  shatterNormal(std::move(shatterNormal)) {
	size = static_cast<float>(this->shatter->getWidth());
}

MaterialGround::~MaterialGround() {
}

Shader* MaterialGround::bind() {
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	// Phase 4: opaque restores depth write here (was Renderer::drawSun).
	glDepthMask(GL_TRUE);
	glDisable(GL_CULL_FACE);
	glDisable(GL_BLEND);

	shader->bind();
	shader->transmitUniform("tileSize", size);
	shader->transmitUniform("heightRange", heightRange);

	moss->bind();
	earth->bind();
	shatter->bind();
	mossNormal->bind();
	earthNormal->bind();
	shatterNormal->bind();
	shader->transmitUniform("texMoss", moss.get());
	shader->transmitUniform("texEarth", earth.get());
	shader->transmitUniform("texShatter", shatter.get());
	shader->transmitUniform("texNormalMoss", mossNormal.get());
	shader->transmitUniform("texNormalEarth", earthNormal.get());
	shader->transmitUniform("texNormalShatter", shatterNormal.get());

	return shader;
}

void MaterialGround::setHeightRange(float f) { heightRange = f; }

void MaterialGround::setAlphaCutoff(float f) { alphaCutoff = f; }
