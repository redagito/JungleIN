#pragma once

#include <unordered_map>
#include <string>

#include "render/Shader.h"

/*!
 * ShaderLibrary
 * Stores the pointers to the different shaders.
 * Use ShaderLibrary::addShader to load a new shader.
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