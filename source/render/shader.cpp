#include "shader.h"

#include <iostream>

#include <flextGL.h>

#include "helpers/utils.h"

Shader::Shader(std::string name) {
  this->name = name;
  this->uid = -1;
  this->vertex = NULL;
  this->fragment = NULL;
}

Shader::~Shader() {
  attributes.clear();
  uniforms.clear();
  delete[] vertex;
  delete[] fragment;
}

Shader &Shader::load(std::string shaderPrefix,
                     std::vector<std::string> attributes,
                     std::vector<std::string> uniforms) {
  ;
  vertex = Utils::getFileContent(shaderPrefix + ".vs");
  fragment = Utils::getFileContent(shaderPrefix + ".fs");

  setup();
  setupLocations(attributes, uniforms);

  return *this;
}

Shader &Shader::setup() {
  if (vertex == NULL || fragment == NULL) {
    std::cout << "Error: Unable to load shader " << name << std::endl;
    exit(-1);
  }

  int status, logSize;
  char log[1024];
  unsigned int pProgram;

  pProgram = glCreateProgram();

  unsigned int vshader = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vshader, 1, &vertex, NULL);
  glCompileShader(vshader);
  glGetShaderiv(vshader, GL_COMPILE_STATUS, &status);
  if (status != GL_TRUE) {
    glGetShaderiv(vshader, GL_INFO_LOG_LENGTH, &logSize);
    glGetShaderInfoLog(vshader, logSize, &logSize, log);
    std::cout << "Error: Unable to compile vertex shader of " << name
              << std::endl;
    std::cout << log << std::endl;
    exit(-1);
  }
  glAttachShader(pProgram, vshader);

  unsigned int fshader = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fshader, 1, &fragment, NULL);
  glCompileShader(fshader);
  if (status != GL_TRUE) {
    glGetShaderiv(fshader, GL_INFO_LOG_LENGTH, &logSize);
    glGetShaderInfoLog(fshader, logSize, &logSize, log);
    std::cout << "Error: Unable to compile fragment shader of " << name
              << std::endl;
    std::cout << log << std::endl;
    exit(-1);
  }
  glAttachShader(pProgram, fshader);

  glLinkProgram(pProgram);
  glGetProgramiv(pProgram, GL_LINK_STATUS, &status);
  if (status != GL_TRUE) {
    glGetProgramiv(pProgram, GL_INFO_LOG_LENGTH, &logSize);
    glGetProgramInfoLog(pProgram, logSize, &logSize, log);
    std::cout << "Error: Unable to link program shader of " << name
              << std::endl;
    std::cout << log << std::endl;
    exit(-1);
  }

  uid = pProgram;

  return *this;
}

Shader &Shader::setupLocations(std::vector<std::string> _attributes,
                               std::vector<std::string> _uniforms) {
  bind();

  for (std::string attribute : _attributes) {
    attributes[attribute] = glGetAttribLocation(uid, attribute.c_str());
  }

  for (std::string uniform : _uniforms) {
    uniforms[uniform] = glGetUniformLocation(uid, uniform.c_str());
  }
  return *this;
}

Shader &Shader::bind() {
  glUseProgram(uid);

  return *this;
}

std::string &Shader::getName() { return name; }

unsigned int Shader::attribute(std::string name) { return attributes.at(name); }

unsigned int Shader::uniform(std::string name) { return uniforms.at(name); }

bool Shader::hasAttribute(std::string name) {
  return attributes.count(name) != 0;
}

bool Shader::hasUniform(std::string name) { return uniforms.count(name) != 0; }

void Shader::transmitUniform(std::string name, const Texture *tex) {
  glUniform1i(value(uniforms, name, -1), tex->index);
}

void Shader::transmitUniform(std::string name, const TextureCube *tex) {
  glUniform1i(value(uniforms, name, -1), tex->index);
}

void Shader::transmitUniform(std::string name, int i) {
  glUniform1i(value(uniforms, name, -1), i);
}

void Shader::transmitUniform(std::string name, float f) {
  glUniform1f(value(uniforms, name, -1), f);
}

void Shader::transmitUniform(std::string name, float f1, float f2) {
  glUniform2f(value(uniforms, name, -1), f1, f2);
}

void Shader::transmitUniform(std::string name, float f1, float f2, float f3) {
  glUniform3f(value(uniforms, name, -1), f1, f2, f3);
}

void Shader::transmitUniform(std::string name, const Vector3 &vec3) {
  glUniform3f(value(uniforms, name, -1), vec3.x, vec3.y, vec3.z);
}

void Shader::transmitUniform(std::string name, const Matrix3 &mat3) {
  glUniformMatrix3fv(value(uniforms, name, -1), 1, GL_TRUE, mat3.array);
}

void Shader::transmitUniform(std::string name, const Matrix4 &mat4) {
  glUniformMatrix4fv(value(uniforms, name, -1), 1, GL_TRUE, mat4.array);
}

void Shader::transmitUniform(std::string name, bool b) {
  glUniform1i(value(uniforms, name, -1), b ? 1 : 0);
}
