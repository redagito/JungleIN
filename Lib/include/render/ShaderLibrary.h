#pragma once

#include <mutex>
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

  // Startup self-check: every registered uniform must resolve to a real
  // location. Registration is compiled in while shader sources load from
  // data/ at runtime, so any skew between the two must fail here with a
  // clear message instead of mid-frame at first draw.
  static void validateRegistrations();

 private:
  static std::unordered_map<std::string, Shader *> shaders;
  static std::mutex mutex_;
};