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
	std::optional<glm::vec3> randomMapPos(double height = 0, bool isRock = false);
	bool containInBoundingBox(double x, double z);
	std::vector<glm::vec3> getRockPos();
	void removeRockPos();

	static const float MAP_SIZE;
	static float MAP_MIN;
	static float MAP_MAX;
	static const float MAX_HEIGHT;
};