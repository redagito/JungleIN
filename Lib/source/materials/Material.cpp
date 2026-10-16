#include "materials/Material.h"
#include "render/ShaderLibrary.h"

Material::Material(std::string shader) {
  this->shader = ShaderLibrary::getShader(shader);
}

Material::Material(Shader *shader) { this->shader = shader; }

Shader *Material::getShader() { return this->shader; }

Shader *Material::bind() { return shader; }

bool Material::hasAlpha() const { return false; }

Texture *Material::getAlpha() { return nullptr; }

float Material::getAlphaCutoff() const { return 0.8f; }
