#pragma once

#include <memory>
#include <vector>

#include "objects/Object3D.h"
#include "materials/Material.h"
#include "core/Geometry.h"
#include "objects/Instance.h"

#include <glad/gl.h>

/*!
 * Mesh extends Object3D
 * A 3D object holding a geometry and a material.
 * Ownership: geometry, material and instances are owned here (unique_ptr).
 * Getters return borrowed raw pointers/views; passing an object in
 * transfers ownership to the Mesh.
 */
class Mesh : public Object3D {
public:
  Mesh();
  Mesh(std::unique_ptr<Geometry> geometry, std::unique_ptr<Material> material);
  ~Mesh();

  void initVBO();
  void drawWithVBO();
  void debugDrawWithVBO();

  // Uploads per-instance world matrices for one instanced draw. The
  // shader resolves instanceModel locations (divisor 1 on four slots);
  // a missing attribute throws like the initVBO lookups do. Reuses the
  // GL buffer across frames (reallocates only on growth). VAO must
  // exist (initVBO first). Models are copied during the call.
  void uploadInstances(const glm::mat4* models, size_t count,
    Shader* shader);
  // Single glDrawElementsInstanced over the uploaded instances.
  // Throws without geometry/initVBO like drawWithVBO; count 0 no-ops.
  void drawInstanced(size_t count);

  // Borrowed: the Mesh keeps ownership. Never null after initVBO.
  Geometry *getGeometry();
  const Geometry *getGeometry() const;
  Material *getMaterial();
  const Material *getMaterial() const;

  void castsShadows(bool active);
  bool castsShadows() const;

  // Creates an owned instance, returns it borrowed.
  Instance *newInstance();
  void clearInstances();
  // Drops the most recently created instance (failed scatter spots).
  // Throwing on empty: popping nothing is a caller bug, not a no-op.
  void discardLastInstance();
  Instance *getInstance(int id);
  const Instance *getInstance(int id) const;
  // Borrowed snapshot views; do not store (rebuilt per call).
  std::vector<Instance *> getInstances();
  std::vector<const Instance *> getInstances() const;
  float getScaleRdn() const;
  void setScaleRdn(float sRdn);
  float getHeightRdn() const;
  void setHeightRdn(float height);
  float getPourcentage() const;
  void setPourcentage(float pct);
  float getRangeScale() const;
  void setRangeScale(float range);

  void setInstanceType(unsigned int i);
  unsigned int getInstanceType() const;

  static const unsigned int INSTANCE_NONE;
  static const unsigned int INSTANCE_PALM;
  static const unsigned int INSTANCE_HTREE;
  static const unsigned int INSTANCE_BTREE;
  static const unsigned int INSTANCE_GPALM;
  static const unsigned int INSTANCE_BUSH;
  static const unsigned int INSTANCE_ROCK;

protected:
  std::unique_ptr<Geometry> geometry;
  std::unique_ptr<Material> material;

  GLuint vertices = 0;
  GLuint indices = 0;
  GLuint normals = 0;
  GLuint texcoords = 0;
  GLuint VAO = 0;

  // Instanced-draw buffer for per-instance world matrices (divisor 1).
  // Grows on demand, never shrinks: regenerate (R key) reuses it.
  GLuint instanceVBO = 0;
  size_t instanceVBOCapacity = 0;

  bool hasNormals = false;
  bool hasTexCoords = false;

  bool castShadows = false;

  // Attribute locations are -1 until initVBO resolves them against the
  // material shader; drawing or transmitting with an unset location
  // throws instead of aliasing position's slot 0.
  int locVertices = -1;
  int locNormals = -1;
  int locTexCoords = -1;

  float scaleRdn = 1.f;
  float rangeScale = 1.f;
  float heightRdn = 1.f;
  float pourcentage = 1.f;
  unsigned int instanceType = 0;

  std::vector<std::unique_ptr<Instance>> instances;

private:
  void initVBOOrThrow();
  void cleanupVBO();
};
