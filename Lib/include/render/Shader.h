#pragma once

#include <string>
#include <vector>
#include <unordered_map>

#include <glad/gl.h>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "core/Texture.h"
#include "core/TextureCube.h"

/*!
 * Shader
 * Stores a glProgram (vertex+fragment).
 * Use bind to use the shader when rendering.
 * Use attribute(string) and uniform(string) to access the locations.
 */
class Shader {
public:
	Shader(std::string name);
	~Shader();

	Shader& load(std::string shader, std::vector<std::string> attributes,
		std::vector<std::string> uniforms);
	Shader& bind();
	Shader& setup();
	Shader& setupLocations(std::vector<std::string> _attributes,
		std::vector<std::string> _uniforms);

	std::string& getName();

	unsigned int attribute(std::string name);
	unsigned int uniform(std::string name);

	// Registered uniform names, for diagnostics and tests (e.g. asserting
	// every registration resolves to a real location).
	std::vector<std::string> uniformNames() const;

	bool hasAttribute(std::string name);
	bool hasUniform(std::string name);

	// Location lookup for hot loops: throws exactly like transmitting
	// to the uniform (same fail-fast error when unregistered or
	// optimized out), without transmitting. Resolve once per material
	// bind and reuse the int with glUniform* in per-instance loops;
	// string-keyed map lookup per draw showed up hot.
	// Call only with this program bound (same rule as transmitUniform).
	int requireLocation(const std::string& key) const;

	void transmitUniform(std::string name, const Texture* tex);
	void transmitUniform(std::string name, const TextureCube* tex);
	void transmitUniform(std::string name, int i);
	void transmitUniform(std::string name, float f);
	void transmitUniform(std::string name, float f1, float f2);
	void transmitUniform(std::string name, float f1, float f2, float f3);
	void transmitUniform(std::string name, const glm::vec3& vec3);
	void transmitUniform(std::string name, const glm::mat3& mat3);
	void transmitUniform(std::string name, const glm::mat4& mat4);
	void transmitUniform(std::string name, bool b);

	// Forgets which program is active. ShaderLibrary::clear calls it:
	// deleting every program while keeping a stale "active" uid would
	// make the next bind() skip glUseProgram and draw with no program.
	static void resetActiveShader();
private:
	// Throws if current shader is not active
	void validateIsActive();

	// Location lookup that fails fast: an unregistered uniform name is a
	// wiring bug, and transmitting to location -1 would silently no-op.
	int requireUniform(const std::string& key) const;

	static GLuint s_activeShaderUid;
	std::string name;
	GLuint uid = 0;
	std::string vertex;
	std::string fragment;

	std::unordered_map<std::string, int> attributes;
	std::unordered_map<std::string, int> uniforms;
};
