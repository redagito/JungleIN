#ifndef MATERIAL_H
#define MATERIAL_H

#include "../render/shader.h"

/*!
 * Material
 * Un material correspond � un shader et contient une structure d'attributs pour
 * stocker ses param�tres.
 * @abstract
 */
class Material {
public:
  Material();

private:
  Shader *shader;
};

#endif // MATERIAL_H
