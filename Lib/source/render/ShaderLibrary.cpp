#include "render/ShaderLibrary.h"

#include <mutex>
#include <stdexcept>

#include "helpers/Logger.h"

#include "helpers/Utils.h"

std::unordered_map<std::string, Shader*> ShaderLibrary::shaders;
std::mutex ShaderLibrary::mutex_;

Shader* ShaderLibrary::getShader(std::string name) {
	std::lock_guard<std::mutex> lock(mutex_);
	if (shaders.count(name) != 0) {
		return shaders.at(name);
	}
	throw std::runtime_error("Shader '" + name +
		"' doesn't exist in ShaderLibrary.");
}

Shader* ShaderLibrary::addShader(std::string name, std::string shaderPrefix,
	std::vector<std::string> attributes,
	std::vector<std::string> uniforms) {
	std::lock_guard<std::mutex> lock(mutex_);
	if (shaders.count(name) != 0) {
		logi("Warning: Duplicate shader '{}' in ShaderLibrary.", name);
		return shaders.at(name);
	}

	Shader* shader = new Shader(name);
	try {
		shader->load(shaderPrefix, attributes, uniforms);
	}
	catch (...) {
		delete shader;
		throw;
	}

	shaders[name] = shader;

	return shader;
}

void ShaderLibrary::clear() {
	std::lock_guard<std::mutex> lock(mutex_);
	for (auto entry : shaders) {
		delete entry.second;
	}
	shaders.clear();
	// Program names die with their GL context, but this map (and the
	// active-program cache) outlive it: without the reset, a recycled
	// uid would make the next bind() skip glUseProgram entirely.
	Shader::resetActiveShader();
}

void ShaderLibrary::validateRegistrations() {
	std::lock_guard<std::mutex> lock(mutex_);
	for (const auto& entry : shaders) {
		for (const std::string& uniform : entry.second->uniformNames()) {
			if (!entry.second->hasUniform(uniform))
				throw std::runtime_error("Shader '" + entry.first +
					"' registration has no location for uniform '" + uniform +
					"' (shader/registration skew?)");
		}
	}
}
