#include "materials/MaterialGround.h"

MaterialGround::MaterialGround(Texture* moss, Texture* earth, Texture* shatter,
	Texture* mossNormal, Texture* earthNormal,
	Texture* shatterNormal)
	: Material("rt_ground_n") {
	this->moss = moss;
	this->earth = earth;
	this->shatter = shatter;
	this->mossNormal = mossNormal;
	this->earthNormal = earthNormal;
	this->shatterNormal = shatterNormal;
	size = (float)shatter->getWidth();
}

MaterialGround::~MaterialGround() {
	delete moss;
	delete earth;
	delete shatter;
	delete mossNormal;
	delete earthNormal;
	delete shatterNormal;
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
	// No-op until the ground shader declares it; keeps the per-material
	// alpha-test threshold plumbed like MaterialBasic.
	if (shader->hasUniform("alphaCutoff"))
		shader->transmitUniform("alphaCutoff", alphaCutoff);

	moss->bind();
	earth->bind();
	shatter->bind();
	mossNormal->bind();
	earthNormal->bind();
	shatterNormal->bind();
	shader->transmitUniform("texMoss", moss);
	shader->transmitUniform("texEarth", earth);
	shader->transmitUniform("texShatter", shatter);
	shader->transmitUniform("texNormalMoss", mossNormal);
	shader->transmitUniform("texNormalEarth", earthNormal);
	shader->transmitUniform("texNormalShatter", shatterNormal);

	return shader;
}

void MaterialGround::setHeightRange(float f) { heightRange = f; }

void MaterialGround::setAlphaCutoff(float f) { alphaCutoff = f; }
