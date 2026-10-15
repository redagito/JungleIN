#pragma once

#include <glm/glm.hpp>

#include "materials/Material.h"

class MaterialBasic : public Material
{
public:
	MaterialBasic(const glm::vec3& _color = glm::vec3(0.0f));
	~MaterialBasic() override;

	void setColor(const glm::vec3& _color);
	glm::vec3& getColor();

	void setDiffuse(Texture* tex);
	void setAlpha(Texture* tex);
	void setAlphaCutoff(float f);

	bool hasAlpha() const override;

	Texture* getAlpha() override;

	Shader* bind();

protected:
	glm::vec3 color;
	Texture* diffuse = nullptr;
	Texture* alpha = nullptr;
	bool hDiffuse = false;
	bool hAlpha = false;
	float alphaCutoff = 0.8f;
};