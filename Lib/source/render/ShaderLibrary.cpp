#include "render/ShaderLibrary.h"

#include <stdexcept>

#include <fmtlog/fmtlog.h>

#include "helpers/Utils.h"

std::unordered_map<std::string, Shader*> ShaderLibrary::shaders;

Shader* ShaderLibrary::getShader(std::string name) {
	if (shaders.count(name) != 0) {
		return shaders.at(name);
	}
	throw std::runtime_error("Shader '" + name +
		"' doesn't exist in ShaderLibrary.");
}

Shader* ShaderLibrary::addShader(std::string name, std::string shaderPrefix,
	std::vector<std::string> attributes,
	std::vector<std::string> uniforms) {
	if (shaders.count(name) != 0) {
		logi("Warning: Duplicate shader '{}' in ShaderLibrary.", name);
		return shaders.at(name);
	}

	Shader* shader = new Shader(name);
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
