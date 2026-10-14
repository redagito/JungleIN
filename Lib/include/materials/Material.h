#ifndef MATERIAL_H
#define MATERIAL_H

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
  virtual ~Material() {}

  Shader *getShader();

  virtual Shader *bind() { return shader; }

protected:
  Shader *shader;
};

#endif // MATERIAL_H
