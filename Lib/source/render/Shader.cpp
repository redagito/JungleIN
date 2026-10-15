#include "render/Shader.h"

#include <stdexcept>
#include <string>
#include <vector>
#include "helpers/Logger.h"

#include "helpers/Utils.h"

namespace {
// Drivers report warnings and remarks through the info log even when
// compilation/linking succeeds (status stays GL_TRUE). A non-empty log
// means the driver had something to say, so surface it instead of
// dropping it like the success path used to do.
std::string fetchInfoLog(unsigned int object, bool isProgram) {
	int length = 0;
	if (isProgram)
		glGetProgramiv(object, GL_INFO_LOG_LENGTH, &length);
	else
		glGetShaderiv(object, GL_INFO_LOG_LENGTH, &length);
	if (length <= 1)
		return {};
	std::string text(static_cast<size_t>(length), '\0');
	if (isProgram)
		glGetProgramInfoLog(object, length, nullptr, text.data());
	else
		glGetShaderInfoLog(object, length, nullptr, text.data());
	while (!text.empty() && text.back() == '\0')
		text.pop_back();
	return text;
}
}

GLuint Shader::s_activeShaderUid = 0;

Shader::Shader(std::string name) {
	this->name = name;
	this->uid = static_cast<GLuint>(-1);
	this->vertex = NULL;
	this->fragment = NULL;
}

Shader::~Shader() {
	attributes.clear();
	uniforms.clear();
	delete[] vertex;
	delete[] fragment;
}

Shader& Shader::load(std::string shaderPrefix,
	std::vector<std::string> _attributes,
	std::vector<std::string> _uniforms)
{
	vertex = Utils::getFileContent(shaderPrefix + ".vs");
	fragment = Utils::getFileContent(shaderPrefix + ".fs");

	setup();
	setupLocations(_attributes, _uniforms);

	return *this;
}

Shader& Shader::setup() {
	if (vertex == NULL || fragment == NULL) {
		logi("Error: Unable to load shader {}", name);
		throw std::runtime_error("Failed to load shader");
	}

	int status, logSize;
	std::vector<char> log;
	unsigned int pProgram;

	pProgram = glCreateProgram();

	unsigned int vshader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vshader, 1, &vertex, NULL);
	glCompileShader(vshader);
	glGetShaderiv(vshader, GL_COMPILE_STATUS, &status);
	if (status != GL_TRUE) {
		glGetShaderiv(vshader, GL_INFO_LOG_LENGTH, &logSize);
		log.assign(logSize > 0 ? logSize : 1, '\0');
		glGetShaderInfoLog(vshader, logSize, &logSize, log.data());
		loge("Error: Unable to compile vertex shader of {}", name);
		loge("{}", log.data());
		throw std::runtime_error("Shader setup failed");
	}
	glAttachShader(pProgram, vshader);
	if (std::string info = fetchInfoLog(vshader, false); !info.empty())
		logw("Vertex shader info log of {}: {}", name, info);

	unsigned int fshader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fshader, 1, &fragment, NULL);
	glCompileShader(fshader);
	glGetShaderiv(fshader, GL_COMPILE_STATUS, &status);
	if (status != GL_TRUE) {
		glGetShaderiv(fshader, GL_INFO_LOG_LENGTH, &logSize);
		log.assign(logSize > 0 ? logSize : 1, '\0');
		glGetShaderInfoLog(fshader, logSize, &logSize, log.data());
		loge("Error: Unable to compile fragment shader of {}", name);
		loge("{}", log.data());
		throw std::runtime_error("Shader setup failed");
	}
	glAttachShader(pProgram, fshader);
	if (std::string info = fetchInfoLog(fshader, false); !info.empty())
		logw("Fragment shader info log of {}: {}", name, info);

	glLinkProgram(pProgram);
	glGetProgramiv(pProgram, GL_LINK_STATUS, &status);
	if (status != GL_TRUE) {
		glGetProgramiv(pProgram, GL_INFO_LOG_LENGTH, &logSize);
		log.assign(logSize > 0 ? logSize : 1, '\0');
		glGetProgramInfoLog(pProgram, logSize, &logSize, log.data());
		loge("Error: Unable to link program shader of {}", name);
		loge("{}", log.data());
		throw std::runtime_error("Shader setup failed");
	}

	uid = pProgram;
	if (std::string info = fetchInfoLog(pProgram, true); !info.empty())
		logw("Program info log of {}: {}", name, info);

	return *this;
}

Shader& Shader::setupLocations(std::vector<std::string> _attributes,
	std::vector<std::string> _uniforms)
{
	bind();

	for (std::string attributeName : _attributes) {
		attributes[attributeName] = glGetAttribLocation(uid, attributeName.c_str());
	}

	for (std::string uniformName : _uniforms) {
		uniforms[uniformName] = glGetUniformLocation(uid, uniformName.c_str());
	}
	return *this;
}

Shader& Shader::bind() {
	if (s_activeShaderUid == uid) return *this;
	glUseProgram(uid);
	s_activeShaderUid = uid;

	return *this;
}

std::string& Shader::getName() { return name; }

unsigned int Shader::attribute(std::string key) { return attributes.at(key); }

unsigned int Shader::uniform(std::string key) { return uniforms.at(key); }

bool Shader::hasAttribute(std::string key) {
	// A registered name optimized out by the linker (location -1) is not
	// usable: treating it as present would feed 4294967295 to GL calls.
	return attributes.count(key) != 0 && attributes.at(key) != -1;
}

bool Shader::hasUniform(std::string key) { return uniforms.count(key) != 0; }

void Shader::transmitUniform(std::string key, const Texture* tex) {
	validateIsActive();
	glUniform1i(value(uniforms, key, -1), tex->index);
}

void Shader::transmitUniform(std::string key, const TextureCube* tex) {
	validateIsActive();
	glUniform1i(value(uniforms, key, -1), tex->index);
}

void Shader::transmitUniform(std::string key, int i) {
	validateIsActive();
	glUniform1i(value(uniforms, key, -1), i);
}

void Shader::transmitUniform(std::string key, float f) {
	validateIsActive();
	glUniform1f(value(uniforms, key, -1), f);
}

void Shader::transmitUniform(std::string key, float f1, float f2) {
	validateIsActive();
	glUniform2f(value(uniforms, key, -1), f1, f2);
}

void Shader::transmitUniform(std::string key, float f1, float f2, float f3) {
	validateIsActive();
	glUniform3f(value(uniforms, key, -1), f1, f2, f3);
}

void Shader::transmitUniform(std::string key, const Vector3& vec3) {
	validateIsActive();
	glUniform3f(value(uniforms, key, -1), vec3.x, vec3.y, vec3.z);
}

void Shader::transmitUniform(std::string key, const Matrix3& mat3) {
	validateIsActive();
	glUniformMatrix3fv(value(uniforms, key, -1), 1, GL_TRUE, mat3.array);
}

void Shader::transmitUniform(std::string key, const Matrix4& mat4) {
	validateIsActive();
	glUniformMatrix4fv(value(uniforms, key, -1), 1, GL_TRUE, mat4.array);
}

void Shader::transmitUniform(std::string key, bool b) {
	validateIsActive();
	glUniform1i(value(uniforms, key, -1), b ? 1 : 0);
}

void Shader::validateIsActive()
{
	if (s_activeShaderUid != uid)
		throw std::runtime_error("Current shader is not active");
}