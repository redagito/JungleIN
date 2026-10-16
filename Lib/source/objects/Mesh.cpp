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
	hasNormals = false;
	hasTexCoords = false;

	castShadows = true;
	this->instanceType = INSTANCE_NONE;
}

Mesh::Mesh(std::unique_ptr<Geometry> geometry, std::unique_ptr<Material> material) : Object3D(),
  geometry(std::move(geometry)), material(std::move(material)) {

	castShadows = true;

	this->instanceType = INSTANCE_NONE;
}

void Mesh::initVBO() {
	if (geometry == nullptr)
		throw std::runtime_error("Mesh::initVBO without geometry");
	if (material == nullptr)
		throw std::runtime_error("Mesh::initVBO without material");
	try {
	initVBOOrThrow();
	}
	catch (...) {
		cleanupVBO();
		throw;
	}
}

void Mesh::cleanupVBO() {
	if (VAO != 0) {
		glDeleteVertexArrays(1, &VAO);
		VAO = 0;
	}
	if (vertices != 0) {
		glDeleteBuffers(1, &vertices);
		vertices = 0;
	}
	if (indices != 0) {
		glDeleteBuffers(1, &indices);
		indices = 0;
	}
	if (normals != 0) {
		glDeleteBuffers(1, &normals);
		normals = 0;
	}
	if (texcoords != 0) {
		glDeleteBuffers(1, &texcoords);
		texcoords = 0;
	}
	locVertices = -1;
	locNormals = -1;
	locTexCoords = -1;
}

void Mesh::initVBOOrThrow() {
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
	locVertices = static_cast<int>(material->getShader()->attribute("position"));
	if (locVertices < 0)
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
		locNormals = static_cast<int>(material->getShader()->attribute("normal"));
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
		locTexCoords = static_cast<int>(material->getShader()->attribute("texcoord"));
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
	if (vertices == 0)
		throw std::runtime_error("Mesh::initVBO vertex buffer not created");
	glBindBuffer(GL_ARRAY_BUFFER, vertices);

	glVertexAttribPointer(locVertices, 3, GL_FLOAT, GL_FALSE, 0, 0);
	if (hasNormals) {
		if (normals == 0)
			throw std::runtime_error("Mesh::initVBO normal buffer not created");
		glEnableVertexAttribArray(locNormals);

		glBindBuffer(GL_ARRAY_BUFFER, normals);
		glVertexAttribPointer(locNormals, 3, GL_FLOAT, GL_FALSE, 0, 0);

	}
	if (hasTexCoords) {
		if (texcoords == 0)
			throw std::runtime_error("Mesh::initVBO texcoord buffer not created");
		glEnableVertexAttribArray(locTexCoords);

		glBindBuffer(GL_ARRAY_BUFFER, texcoords);
		glVertexAttribPointer(locTexCoords, 2, GL_FLOAT, GL_FALSE, 0, 0);
	}

	if (indices == 0)
		throw std::runtime_error("Mesh::initVBO index buffer not created");
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indices);

	glBindVertexArray(0);
}

void Mesh::drawWithVBO() {

	if (geometry == nullptr)
		throw std::runtime_error("Mesh::drawWithVBO without geometry");
	if (VAO == 0)
		throw std::runtime_error("Mesh::drawWithVBO without initVBO");

	glBindVertexArray(VAO);
	glDrawElements(geometry->getPrimitive(), geometry->getIndicesCount(),
		GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);
}
void Mesh::debugDrawWithVBO() {

	if (geometry == nullptr)
		throw std::runtime_error("Mesh::debugDrawWithVBO without geometry");
	if (VAO == 0)
		throw std::runtime_error("Mesh::debugDrawWithVBO without initVBO");

	glBindVertexArray(VAO);

	glDrawElements(GL_POINTS, geometry->getIndicesCount(),
		GL_UNSIGNED_INT, 0);

	glBindVertexArray(0);
}

Geometry* Mesh::getGeometry() { return geometry.get(); }

const Geometry* Mesh::getGeometry() const { return geometry.get(); }

Material* Mesh::getMaterial() { return material.get(); }

const Material* Mesh::getMaterial() const { return material.get(); }

void Mesh::castsShadows(bool active) { castShadows = active; }

bool Mesh::castsShadows() const { return castShadows; }

Instance* Mesh::newInstance() {
	instances.push_back(std::make_unique<Instance>(this,
		static_cast<int>(instances.size())));

	return instances.back().get();
}

void Mesh::clearInstances() {
	instances.clear();
}

void Mesh::discardLastInstance() {
	if (instances.empty())
		throw std::runtime_error("Mesh::discardLastInstance on empty set");
	instances.pop_back();
}

Instance* Mesh::getInstance(int id) { return instances.at(id).get(); }

const Instance* Mesh::getInstance(int id) const { return instances.at(id).get(); }

std::vector<Instance*> Mesh::getInstances() {
	std::vector<Instance*> view;
	for (const auto& instance : instances)
		view.push_back(instance.get());
	return view;
}

std::vector<const Instance*> Mesh::getInstances() const {
	std::vector<const Instance*> view;
	for (const auto& instance : instances)
		view.push_back(instance.get());
	return view;
}

float Mesh::getScaleRdn() const { return scaleRdn; }

void Mesh::setScaleRdn(float sRdn) { this->scaleRdn = sRdn; }

float Mesh::getHeightRdn() const { return this->heightRdn; }

Mesh::~Mesh() {
	cleanupVBO();
}

void Mesh::setPourcentage(float pct) { this->pourcentage = pct; }

float Mesh::getRangeScale() const { return rangeScale; }

void Mesh::setRangeScale(float range) { this->rangeScale = range; }

void Mesh::setInstanceType(unsigned int i) { this->instanceType = i; }

float Mesh::getPourcentage() const { return this->pourcentage; }

void Mesh::setHeightRdn(float height) { this->heightRdn = height; }

unsigned int Mesh::getInstanceType() const { return this->instanceType; }
