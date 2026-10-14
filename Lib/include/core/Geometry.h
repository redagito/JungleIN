#pragma once

#include <vector>

#include <glad/gl.h>

/*!
 * Geometry
 * Data structure for storing vertex, index,
 * etc.
 */
class Geometry {
public:
  //  static const unsigned int TRIANGLES;
  //  static const unsigned int QUADS;
  //  static const unsigned int POINTS;

  Geometry(float *vertices, int *indices, int numVertices, int numIndices);
  Geometry(std::vector<float> vertices, std::vector<unsigned int> indices);

  float *getVertices();
  unsigned int *getIndices();
  float *getTexCoords();
  float *getNormals();

  int getVerticesCount();
  int getIndicesCount();
  int getTexCoordsCount();

  void setTexCoords(float *texCoords);
  void setNormals(float *normals);

  void setTexCoords(std::vector<float> texCoords);
  void setNormals(std::vector<float> normals);

  void setPrimitive(GLenum glenum);
  GLenum getPrimitive();

  bool hasNormals();
  bool hasTexCoords();

private:
  std::vector<float> verticesTab;
  std::vector<unsigned int> indicesTab;
  std::vector<float> texCoordsTab;
  std::vector<float> normalsTab;

  GLenum primitive = GL_TRIANGLES;

  int verticesCount = 0;
  int indicesCount = 0;
  int texCoordsCount = 0;

  bool hNormals = false;
  bool hTexCoords = false;
};