#pragma once

#include <math.h>
#include <optional>

#include <glm/glm.hpp>

#include "materials/MaterialGround.h"
#include "objects/Mesh.h"
#include "objects/Path.h"
#include "core/Image.h"

class Ground : public Mesh {

private:
	int numberVerticesX;
	int numberVerticesZ;
	glm::vec3* faceNormals;
	Path* path;
	std::vector<glm::vec3> rockPos;

public:
	Ground(std::string filePath, MaterialGround* mat);
	~Ground();

	void setVectorToFloat(glm::vec3 normal, float* normalsF, int i);
	void firstLineNormals(float* normalsF);
	void middleGridNormals(float* normalsF);
	void firstVertexNormal(float* normalsF, int lineNumber);
	void lastLineNormals(float* normalsF);
	float getY(float x, float z);
	float getYApprox(float x, float z, float prevY);
	// Plane-height solver behind getY, exposed for headless testing: a
	// full Ground needs GL materials, so the guard contract (vertical
	// normal, off-map coordinates) is pinned here instead of on a fixture.
	// Throws std::runtime_error on a vertical normal (|n.y| ~= 0) and
	// std::out_of_range on off-map coordinates.
	static float planeHeightAt(const glm::vec3& p, const glm::vec3& n,
		float x, float z);
	std::optional<glm::vec3> randomMapPos(double height = 0, bool isRock = false);
	bool containInBoundingBox(double x, double z);
	std::vector<glm::vec3> getRockPos();
	void removeRockPos();

	static const float MAP_SIZE;
	static float MAP_MIN;
	static float MAP_MAX;
	static const float MAX_HEIGHT;
};