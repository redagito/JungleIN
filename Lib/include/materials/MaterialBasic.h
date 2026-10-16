#pragma once

#include <glm/glm.hpp>

#include <memory>

#include "materials/Material.h"

class MaterialBasic : public Material
{
public:
	MaterialBasic(const glm::vec3& _color = glm::vec3(0.0f));
	~MaterialBasic() override;

	void setColor(const glm::vec3& _color);
	glm::vec3& getColor();

	void setDiffuse(std::unique_ptr<Texture> tex);
	void setAlpha(std::unique_ptr<Texture> tex);
	void setAlphaCutoff(float f);
	float getAlphaCutoff() const override;

	bool hasAlpha() const override;

	Texture* getAlpha() override;

	Shader* bind();

protected:
	glm::vec3 color;
	// Owned textures (setDiffuse/setAlpha transfer ownership; resetting
	// also fixes the old assign-without-delete leak on replace).
	std::unique_ptr<Texture> diffuse;
	std::unique_ptr<Texture> alpha;
	bool hDiffuse = false;
	bool hAlpha = false;
	float alphaCutoff = 0.8f;
};