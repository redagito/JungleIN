#include "geometry.h"

#include <cstring>

#include <flextGL.h>

const unsigned int Geometry::TRIANGLES = GL_TRIANGLES;
const unsigned int Geometry::QUADS = GL_QUADS;
const unsigned int Geometry::POINTS = GL_POINTS;

Geometry::Geometry(float* vertices, int* indices, int numVertices, int numIndices) {
	verticesTab = std::vector<float>(&vertices[0], &vertices[numVertices]);
	indicesTab = std::vector<unsigned int>(&indices[0], &indices[numIndices]);

	verticesCount = numVertices;
	indicesCount = numIndices;

	hNormals = false;
	hTexCoords = false;

	primitive = Geometry::TRIANGLES;
}

Geometry::Geometry(std::vector<float> vertices, std::vector<unsigned int> indices)
{
	verticesTab = vertices;
  indicesTab = indices;

  verticesCount = vertices.size();
  indicesCount = indices.size();

  hNormals = false;
  hTexCoords = false;

  primitive = Geometry::TRIANGLES;
}

float *Geometry::getVertices() { return verticesTab.data(); }

unsigned int *Geometry::getIndices() { return indicesTab.data(); }

float *Geometry::getTexCoords() { return texCoordsTab.data(); }

float *Geometry::getNormals() { return normalsTab.data(); }

int Geometry::getVerticesCount() { return verticesCount; }

int Geometry::getIndicesCount() { return indicesCount; }

int Geometry::getTexCoordsCount() { return texCoordsCount; }

void Geometry::setTexCoords(float* texCoords) {
	texCoordsCount = verticesCount / 3 * 2;
	texCoordsTab = std::vector<float>(&texCoords[0], &texCoords[verticesCount / 3 * 2]);
	hTexCoords = true;
}

void Geometry::setNormals(float* normals) {
	normalsTab = std::vector<float>(&normals[0], &normals[verticesCount]);
	hNormals = true;
}

void Geometry::setTexCoords(std::vector<float> texCoords) {
	texCoordsCount = texCoords.size();
  texCoordsTab = texCoords;
  hTexCoords = true;
}

void Geometry::setNormals(std::vector<float> normals) {
	normalsTab = normals;
  hNormals = true;
}

bool Geometry::hasNormals() { return hNormals; }

bool Geometry::hasTexCoords() { return hTexCoords; }

void Geometry::setPrimitive(unsigned int glenum) { primitive = glenum; }

unsigned int Geometry::getPrimitive() { return primitive; }
