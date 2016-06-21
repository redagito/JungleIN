#ifndef SHADERLIBRARY_H
#define SHADERLIBRARY_H

#include <unordered_map>
#include <string>

#include "shader.h"

/*!
 * ShaderLibrary
 * Sert � stocker les pointeurs sur les diff�rent shaders.
 * Utiliser ShaderLibrary::addShader pour charger un nouveau shader.
 * @static
 */
class ShaderLibrary {
public:
  static Shader *getShader(std::string name);
  static Shader *addShader(std::string name, std::string shaderPrefix,
                           std::vector<std::string> attributes, std::vector<std::string> uniforms);
  static void clear();

private:
  static std::unordered_map<std::string, Shader *> shaders;
};

#endif // SHADERLIBRARY_H
