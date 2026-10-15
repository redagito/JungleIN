#pragma once

#include "materials/Material.h"

class MaterialBasic : public Material
{
public:
	MaterialBasic(const Vector3& _color = Vector3());
	~MaterialBasic() override;

	void setColor(const Vector3& _color);
	Vector3& getColor();

	void setDiffuse(Texture* tex);
	void setAlpha(Texture* tex);

	bool hasAlpha() const override;

	Texture* getAlpha() override;

	Shader* bind();

protected:
	Vector3 color;
	Texture* diffuse = nullptr;
	Texture* alpha = nullptr;
	bool hDiffuse = false;
	bool hAlpha = false;
};