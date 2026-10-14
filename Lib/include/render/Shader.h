#pragma once

#include <string>
#include <vector>
#include <unordered_map>

#include <glad/glad.h>

#include "math/Vector3.h"
#include "math/Matrix3.h"
#include "math/Matrix4.h"
#include "core/Texture.h"
#include "core/TextureCube.h"

/*!
 * Shader
 * Stock un glProgram (vertex+fragment).
 * Utiliser bind pour utiliser le shader lors du rendu.
 * Utiliser attribute(string) et uniform(string) pour acc�der aux locations.
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

	bool hasAttribute(std::string name);
	bool hasUniform(std::string name);

	void transmitUniform(std::string name, const Texture* tex);
	void transmitUniform(std::string name, const TextureCube* tex);
	void transmitUniform(std::string name, int i);
	void transmitUniform(std::string name, float f);
	void transmitUniform(std::string name, float f1, float f2);
	void transmitUniform(std::string name, float f1, float f2, float f3);
	void transmitUniform(std::string name, const Vector3& vec3);
	void transmitUniform(std::string name, const Matrix3& mat3);
	void transmitUniform(std::string name, const Matrix4& mat4);
	void transmitUniform(std::string name, bool b);

private:
	// Throws if current shader is not active
	void validateIsActive();

	static GLuint s_activeShaderUid;
	std::string name;
	GLuint uid = 0;
	const char* vertex = nullptr;
	const char* fragment = nullptr;

	std::unordered_map<std::string, int> attributes;
	std::unordered_map<std::string, int> uniforms;
};
