#pragma once

#include <vector>

#include "objects/Object3D.h"
#include "materials/Material.h"
#include "core/Geometry.h"
#include "objects/Instance.h"

#include <glad/gl.h>

/*!
 * Mesh extends Object3D
 * A 3D object holding a geometry and a material.
 */
class Mesh : public Object3D {
public:
  Mesh();
  Mesh(Geometry *geometry, Material *material);
  ~Mesh();

  void initVBO();
  void drawWithVBO();
  void debugDrawWithVBO();

  Geometry *getGeometry();
  Material *getMaterial();

  void castsShadows(bool active);
  bool castsShadows();

  Instance *newInstance();
  // void removeInstance(int id);
  void clearInstances();
  Instance *getInstance(int id);
  std::vector<Instance *> &getInstances();
  float getScaleRdn();
  void setScaleRdn(float sRdn);
  float getHeightRdn();
  void setHeightRdn(float height);
  float getPourcentage();
  void setPourcentage(float pct);
  float getRangeScale();
  void setRangeScale(float range);

  void setInstanceType(unsigned int i);
  unsigned int getInstanceType();

  static const unsigned int INSTANCE_NONE;
  static const unsigned int INSTANCE_PALM;
  static const unsigned int INSTANCE_HTREE;
  static const unsigned int INSTANCE_BTREE;
  static const unsigned int INSTANCE_GPALM;
  static const unsigned int INSTANCE_BUSH;
  static const unsigned int INSTANCE_ROCK;

protected:
  Geometry *geometry = nullptr;
  Material *material = nullptr;

  GLuint vertices = 0;
  GLuint indices = 0;
  GLuint normals = 0;
  GLuint texcoords = 0;
  GLuint VAO = 0;

  bool hasNormals = false;
  bool hasTexCoords = false;

  bool castShadows = false;

  unsigned int locVertices = 0;
  unsigned int locNormals = 0;
  unsigned int locTexCoords = 0;

  float scaleRdn = 1.f;
  float rangeScale = 1.f;
  float heightRdn = 1.f;
  float pourcentage = 1.f;
  unsigned int instanceType = 0;

  std::vector<Instance *> instances;
};
