#include "materials/MaterialSun.h"

MaterialSun::MaterialSun(std::unique_ptr<Texture> diffuse,
                         std::unique_ptr<Texture> alpha)
    : Material("rt_sun"), diffuse(std::move(diffuse)),
      alpha(std::move(alpha)) {
}

MaterialSun::~MaterialSun() {
}

Shader *MaterialSun::bind() {
	// Test depth so nearby geometry occludes the sun, but never write it:
	// the sun must not punch holes in what is drawn before it either.
	glEnable(GL_DEPTH_TEST);
	glDepthMask(GL_FALSE);
	glDisable(GL_CULL_FACE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	shader->bind();

	diffuse->bind();
	shader->transmitUniform("texDiffuse", diffuse.get());

	alpha->bind();
	shader->transmitUniform("texAlpha", alpha.get());

	return shader;
}
