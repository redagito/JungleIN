#include "objects/Mesh.h"

#include "helpers/Logger.h"
#include <cassert>
#include <stdexcept>

#include "core/Image.h"

const unsigned int Mesh::INSTANCE_NONE = 0;
const unsigned int Mesh::INSTANCE_PALM = 1;
const unsigned int Mesh::INSTANCE_HTREE = 2;
const unsigned int Mesh::INSTANCE_BTREE = 3;
const unsigned int Mesh::INSTANCE_GPALM = 4;
const unsigned int Mesh::INSTANCE_BUSH = 5;
const unsigned int Mesh::INSTANCE_ROCK = 6;

Mesh::Mesh() : Object3D() {
	this->geometry = 0;
	this->material = 0;

	hasNormals = false;
	hasTexCoords = false;

	castShadows = true;
	this->instanceType = INSTANCE_NONE;
}

Mesh::Mesh(Geometry* geometry, Material* material) : Object3D() {
	this->geometry = geometry;
	this->material = material;

	castShadows = true;

	this->instanceType = INSTANCE_NONE;
}

void Mesh::initVBO() {
	if (geometry == nullptr)
		throw std::runtime_error("Mesh::initVBO without geometry");
	if (material == nullptr)
		throw std::runtime_error("Mesh::initVBO without material");
	hasNormals =
		(geometry->hasNormals() && material->getShader()->hasAttribute("normal"));
	hasTexCoords = (geometry->hasTexCoords() &&
		material->getShader()->hasAttribute("texcoord"));

	int nbV = this->getGeometry()->getVertexFloatCount();
	int nbI = this->getGeometry()->getIndicesCount();
	int nbT = this->getGeometry()->getTexCoordFloatCount();

	// indices
	glGenBuffers(1, &indices);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indices);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, nbI * sizeof(unsigned int), 0, GL_STATIC_DRAW);
	unsigned int* ind =
		(unsigned int*)glMapBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_WRITE_ONLY);
	if (ind == nullptr)
		throw std::runtime_error("Mesh::initVBO failed to map index buffer");
	unsigned int* indData = this->getGeometry()->getIndices();
	for (int i = 0; i < nbI; i++) {
		ind[i] = indData[i];
	}
	glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);

	// vertices
	locVertices = material->getShader()->attribute("position");
	if (locVertices == static_cast<unsigned int>(-1))
		throw std::runtime_error("Mesh::initVBO without position attribute");
	glGenBuffers(1, &vertices);
	glBindBuffer(GL_ARRAY_BUFFER, vertices);
	glBufferData(GL_ARRAY_BUFFER, nbV * sizeof(float), 0, GL_STATIC_DRAW);
	float* vert = (float*)glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
	if (vert == nullptr)
		throw std::runtime_error("Mesh::initVBO failed to map vertex buffer");
	float* vertData = this->getGeometry()->getVertices();
	for (int i = 0; i < nbV; i++) {
		vert[i] = vertData[i];
	}
	glUnmapBuffer(GL_ARRAY_BUFFER);

	if (hasNormals) {
		locNormals = material->getShader()->attribute("normal");
		glGenBuffers(1, &normals);
		glBindBuffer(GL_ARRAY_BUFFER, normals);
		glBufferData(GL_ARRAY_BUFFER, nbV * sizeof(float), 0, GL_STATIC_DRAW);
		float* norm = (float*)glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
		if (norm == nullptr)
			throw std::runtime_error("Mesh::initVBO failed to map normal buffer");
		float* normData = this->getGeometry()->getNormals();
		for (int i = 0; i < nbV; i++) {
			norm[i] = normData[i];
		}
		glUnmapBuffer(GL_ARRAY_BUFFER);
	}

	if (hasTexCoords) {
		locTexCoords = material->getShader()->attribute("texcoord");
		glGenBuffers(1, &texcoords);
		glBindBuffer(GL_ARRAY_BUFFER, texcoords);
		glBufferData(GL_ARRAY_BUFFER, nbT * sizeof(float), 0, GL_STATIC_DRAW);
		float* tex = (float*)glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
		if (tex == nullptr)
			throw std::runtime_error("Mesh::initVBO failed to map texcoord buffer");
		float* texData = this->getGeometry()->getTexCoords();
		for (int i = 0; i < nbT; i++) {
			tex[i] = texData[i];
		}
		glUnmapBuffer(GL_ARRAY_BUFFER);
	}

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	// Setup the VAO
	glGenVertexArrays(1, &VAO);
	glBindVertexArray(VAO);

	glEnableVertexAttribArray(locVertices);
	assert(vertices != 0);
	glBindBuffer(GL_ARRAY_BUFFER, vertices);

	glVertexAttribPointer(locVertices, 3, GL_FLOAT, GL_FALSE, 0, 0);
	if (hasNormals) {
		assert(normals != 0);
		glEnableVertexAttribArray(locNormals);

		glBindBuffer(GL_ARRAY_BUFFER, normals);
		glVertexAttribPointer(locNormals, 3, GL_FLOAT, GL_FALSE, 0, 0);

	}
	if (hasTexCoords) {
		assert(texcoords != 0);
		glEnableVertexAttribArray(locTexCoords);

		glBindBuffer(GL_ARRAY_BUFFER, texcoords);
		glVertexAttribPointer(locTexCoords, 2, GL_FLOAT, GL_FALSE, 0, 0);
	}

	assert(indices != 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indices);

	glBindVertexArray(0);
}

void Mesh::drawWithVBO() {

	if (geometry == nullptr)
		throw std::runtime_error("Mesh::drawWithVBO without geometry");

	glBindVertexArray(VAO);
	glDrawElements(geometry->getPrimitive(), geometry->getIndicesCount(),
		GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);
}
void Mesh::debugDrawWithVBO() {

	if (geometry == nullptr)
		throw std::runtime_error("Mesh::debugDrawWithVBO without geometry");

	glBindVertexArray(VAO);

	glDrawElements(GL_POINTS, geometry->getIndicesCount(),
		GL_UNSIGNED_INT, 0);

	glBindVertexArray(0);
}

Geometry* Mesh::getGeometry() { return geometry; }

Material* Mesh::getMaterial() { return material; }

void Mesh::castsShadows(bool active) { castShadows = active; }

bool Mesh::castsShadows() { return castShadows; }

Instance* Mesh::newInstance() {
	Instance* instance = new Instance(this, static_cast<int>(instances.size()));
	instances.push_back(instance);

	return instance;
}

// void Mesh::removeInstance(int id) {
//  Instance *instance = instances.at(id);
//  if (instance == 0)
//    return;
//
//  delete instance;
//  instances.removeAt(id);
//}

void Mesh::clearInstances() {
	for (Instance* i : instances) {
		delete i;
	}
	instances.clear();
}

Instance* Mesh::getInstance(int id) { return instances.at(id); }

std::vector<Instance*>& Mesh::getInstances() { return instances; }

float Mesh::getScaleRdn() { return scaleRdn; }

void Mesh::setScaleRdn(float sRdn) { this->scaleRdn = sRdn; }

float Mesh::getHeightRdn() { return this->heightRdn; }

Mesh::~Mesh() {
	for (Instance* i : instances) {
		delete i;
	}

	instances.clear();

	if (VAO != 0)
		glDeleteVertexArrays(1, &VAO);
	if (vertices != 0)
		glDeleteBuffers(1, &vertices);
	if (indices != 0)
		glDeleteBuffers(1, &indices);
	if (normals != 0)
		glDeleteBuffers(1, &normals);
	if (texcoords != 0)
		glDeleteBuffers(1, &texcoords);

	delete geometry;
	delete material;
}

void Mesh::setPourcentage(float pct) { this->pourcentage = pct; }

float Mesh::getRangeScale() { return rangeScale; }

void Mesh::setRangeScale(float range) { this->rangeScale = range; }

void Mesh::setInstanceType(unsigned int i) { this->instanceType = i; }

float Mesh::getPourcentage() { return this->pourcentage; }

void Mesh::setHeightRdn(float height) { this->heightRdn = height; }

unsigned int Mesh::getInstanceType() { return this->instanceType; }
