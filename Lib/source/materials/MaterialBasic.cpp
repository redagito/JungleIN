#include "materials/MaterialBasic.h"
#include "helpers/Logger.h"

MaterialBasic::MaterialBasic(const Vector3& _color) : Material("rt_basic") {
	color.copy(_color);
	hDiffuse = false;
	hAlpha = false;
}

MaterialBasic::~MaterialBasic() {
	delete diffuse;
	delete alpha;
}

Shader* MaterialBasic::bind() {
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	// Phase 4: opaque restores depth write here (was Renderer::drawSun).
	glDepthMask(GL_TRUE);
	glDisable(GL_CULL_FACE);
	glDisable(GL_BLEND);

	shader->bind();

	// Per-material alpha-test threshold (default keeps the legacy 0.8 look).
	if (shader->hasUniform("alphaCutoff"))
		shader->transmitUniform("alphaCutoff", alphaCutoff);

	if (hDiffuse) {
		diffuse->bind();
		shader->transmitUniform("texDiffuse", diffuse);
	}

	if (hAlpha) {
		alpha->bind();
		shader->transmitUniform("texAlpha", alpha);
	}

	// Stale-unit guard: rt_basic is shared by every mesh, so a sampler
	// left over from another mesh's draw would sample garbage. Flag the
	// absent maps so the shader never reads an unbound sampler.
	if (shader->hasUniform("useDiffuse"))
		shader->transmitUniform("useDiffuse", hDiffuse ? 1 : 0);
	if (shader->hasUniform("useAlpha"))
		shader->transmitUniform("useAlpha", hAlpha ? 1 : 0);
	if (!hDiffuse && shader->hasUniform("diffuseColor"))
		shader->transmitUniform("diffuseColor", color);

	return shader;
}

void MaterialBasic::setColor(const Vector3& _color) { color.copy(_color); }

Vector3& MaterialBasic::getColor() { return color; }

void MaterialBasic::setDiffuse(Texture* tex) {
	diffuse = tex;
	hDiffuse = true;
}

void MaterialBasic::setAlpha(Texture* tex) {
	alpha = tex;
	hAlpha = true;
}

void MaterialBasic::setAlphaCutoff(float f) { alphaCutoff = f; }

bool MaterialBasic::hasAlpha() const { return hAlpha; }

Texture* MaterialBasic::getAlpha() { return alpha; }
