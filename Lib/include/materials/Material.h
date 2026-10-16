#pragma once

#include "render/Shader.h"

/*!
 * Material
 * A material corresponds to a shader and contains an attribute structure for
 * storing its parameters.
 * @abstract
 */
class Material {
public:
  Material(std::string shader);
  Material(Shader *shader);
  virtual ~Material() = default;

  Shader *getShader();

  virtual Shader *bind();

  virtual bool hasAlpha() const;

  virtual Texture *getAlpha();

  // Per-material alpha-test threshold. The shadow pass transmits the same
  // value as the lit pass so cutout shadows match cutout shading.
  virtual float getAlphaCutoff() const;

protected:
  Shader *shader;
};
