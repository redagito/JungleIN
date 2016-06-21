#ifndef GEOMETRY_H
#define GEOMETRY_H

#include <vector>

/*!
 * Geometry
 * Structure de données permettant de stocker les tableaux de vertices, indices,
 * etc.
 */
class Geometry {
public:
  static const unsigned int TRIANGLES;
  static const unsigned int QUADS;
  static const unsigned int POINTS;

  Geometry(float* vertices, int* indices, int numVertices, int numIndices);
  Geometry(std::vector<float> vertices, std::vector<unsigned int> indices);

  float *getVertices();
  unsigned int *getIndices();
  float *getTexCoords();
  float *getNormals();

  int getVerticesCount();
  int getIndicesCount();
  int getTexCoordsCount();

  void setTexCoords(float* texCoords);
  void setNormals(float* normals);

  void setTexCoords(std::vector<float> texCoords);
  void setNormals(std::vector<float> normals);

  void setPrimitive(unsigned int glenum);
  unsigned int getPrimitive();

  bool hasNormals();
  bool hasTexCoords();

private:
	std::vector<float> verticesTab;
	std::vector<unsigned int> indicesTab;
  std::vector<float> texCoordsTab;
  std::vector<float> normalsTab;

  unsigned int primitive;

  int verticesCount;
  int indicesCount;
  int texCoordsCount;

  bool hNormals;
  bool hTexCoords;
};

#endif // GEOMETRY_H
