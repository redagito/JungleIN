#include "shaderlibrary.h"

#include <iostream>

#include "helpers/utils.h"

std::unordered_map<std::string, Shader *> ShaderLibrary::shaders;

Shader *ShaderLibrary::getShader(std::string name) {
  if (shaders.count(name) != 0) {
    return shaders.at(name);
  }
  std::cout << "Error : Shader '" << name << "' doesn't exist in ShaderLibrary."
            << std::endl;
  exit(-1);
  return nullptr;
}

Shader *ShaderLibrary::addShader(std::string name, std::string shaderPrefix,
                                 std::vector<std::string> attributes,
                                 std::vector<std::string> uniforms) {
  if (shaders.count(name) != 0) {
    std::cout << "Warning: Duplicate shader '" << name << "' in ShaderLibrary."
              << std::endl;
    return shaders.at(name);
  }

  Shader *shader = new Shader(name);
  shader->load(shaderPrefix, attributes, uniforms);

  shaders[name] = shader;

  return shader;
}

void ShaderLibrary::clear() {
  for (auto entry : shaders) {
    delete entry.second;
  }
  shaders.clear();
}
