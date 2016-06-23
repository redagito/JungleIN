#ifndef MESH_H
#define MESH_H

#include <vector>

#include "objects/object3d.h"
#include "materials/material.h"
#include "core/geometry.h"
#include "instance.h"

#include <flextGL.h>

/*!
 * Mesh extends Object3D
 * Un object 3D contenant une g�om�trie et un mat�rial.
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

  bool isInstance();

  static const unsigned int INSTANCE_NONE;
  static const unsigned int INSTANCE_PALM;
  static const unsigned int INSTANCE_HTREE;
  static const unsigned int INSTANCE_BTREE;
  static const unsigned int INSTANCE_GPALM;
  static const unsigned int INSTANCE_BUSH;
  static const unsigned int INSTANCE_ROCK;

protected:
  Geometry *geometry;
  Material *material;

  GLuint vertices = 0;
  GLuint indices = 0;
  GLuint normals = 0;
  GLuint texcoords = 0;

  bool hasNormals = false;
  bool hasTexCoords = false;

  bool castShadows;

  unsigned int locVertices;
  unsigned int locNormals;
  unsigned int locTexCoords;

  float scaleRdn;
  float rangeScale;
  float heightRdn;
  float pourcentage;
  unsigned int instanceType;

  std::vector<Instance *> instances;
};

#endif // MESH_H
