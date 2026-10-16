#include "helpers/Logger.h"

#include "objects/Ground.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <memory>
#include <stdexcept>
#include <vector>

namespace {
// Radial pooling of the heightmap rim (bowl falloff scale) and the
// squared rock-exclusion radius behind containInBoundingBox.
constexpr double kPoolIntensity = 20.0;
constexpr double kRockExclusionRadiusSq = 40.0;

// Zero-length vectors normalize to zero (no NaN); glm does not.
glm::vec3 safeNormalize(const glm::vec3 &v) {
	float len = std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
	if (len > std::numeric_limits<float>::epsilon())
		return v / len;
	return glm::vec3(0.0f);
}
} // namespace

// const used to define the ground's coordinates
const float Ground::MAP_SIZE = 900.0;
const float Ground::MAX_HEIGHT = 10.0;

Ground::Ground(std::string filePath, std::unique_ptr<MaterialGround> mat) : Mesh() {
	// Load the heightmap picture (data map: no flip, QImage origin).
	// The Path mask is allocated only after this succeeds: if the image
	// load throws, there is no half-built Path left behind to leak (the
	// destructor never runs on a failed construction).
	Image img(filePath, false);
	this->path = std::make_unique<Path>("data/resources/path/path.png");
	// init the size of the loaded picture
	this->numberVerticesX = img.getWidth();
	this->numberVerticesZ = img.getHeight();
	if (this->numberVerticesX < 2 || this->numberVerticesZ < 2)
		throw std::runtime_error("Ground heightmap too small (need 2x2 minimum)");

	// init the array's sizes
	std::vector<float> verticesTab(
		static_cast<size_t>(this->numberVerticesX) * this->numberVerticesZ * 3);
	// 6 indices for each quad (made with 2 triangles)
	std::vector<int> indicesTab(
		static_cast<size_t>(this->numberVerticesX - 1) * (this->numberVerticesZ - 1) * 6);
	// 2 normals for each quad (made with 2 triangles)
	faceNormals.resize(static_cast<size_t>(this->numberVerticesX - 1) *
		static_cast<size_t>(this->numberVerticesZ - 1) * 2);
	// there is 3 coordinates for each normal
	std::vector<float> normalsF(
		static_cast<size_t>(this->numberVerticesX - 1) * (this->numberVerticesZ - 1) * 6);
	// used as array indice
	int i = 0;

	double poolEffect = 0.0;
	double poolIntensity = kPoolIntensity;
	double centerx = this->numberVerticesX / 2.0;
	double centery = this->numberVerticesZ / 2.0;
	double curx, cury;
	double maxlen = std::sqrt(centerx * centerx + centery * centery);

	// get all the vertices from the picture
	// white is the hightest, black is the lowest
	for (int z = 0; z < this->numberVerticesZ; ++z) {
		for (int x = 0; x < this->numberVerticesX; ++x) {
			curx = x - centerx;
			cury = z - centery;
			poolEffect =
				(std::sqrt(curx * curx + cury * cury) / maxlen) * poolIntensity;

			// get the color of the pixel
			auto color = img.getPixel(x, z);
			// the (0,0,0) point is at the center of the map
			verticesTab[i] = (MAP_SIZE * x / (numberVerticesX - 1)) - MAP_SIZE / 2;
		verticesTab[i + 1] =
			static_cast<float>(MAX_HEIGHT * poolEffect * ((color.r + color.g + color.b) / 3) / 255);
			verticesTab[i + 2] =
				(MAP_SIZE * z / (numberVerticesZ - 1)) - MAP_SIZE / 2;

			i += 3;
		}
	}

	i = 0;
	int j = 0;
	// initialize the array of indices
	for (int z = 0; z < this->numberVerticesZ - 1; ++z) {
		for (int x = 0; x < this->numberVerticesX - 1; ++x) {
			int indiceHoriz = x + z * this->numberVerticesX;
			int indiceVer = x + (z + 1) * this->numberVerticesX;
			// construct an indices array of squares, based on 2 triangles
			// first triangle
			indicesTab[i] = indiceHoriz;
			indicesTab[i + 1] = indiceVer;
			indicesTab[i + 2] = indiceHoriz + 1;

			// second triangle
			indicesTab[i + 3] = indiceHoriz + 1;
			indicesTab[i + 4] = indiceVer;
			indicesTab[i + 5] = indiceVer + 1;

		glm::vec3 u;
		glm::vec3 v;
		// make the match between the indices and the vertices' coordinates
		indiceHoriz *= 3;
		indiceVer *= 3;
		// first triangle
		// initialize u vector
		u = glm::vec3(verticesTab[indiceVer] - verticesTab[indiceHoriz],
			verticesTab[indiceVer + 1] - verticesTab[indiceHoriz + 1],
			verticesTab[indiceVer + 2] - verticesTab[indiceHoriz + 2]);
		u = safeNormalize(u);
		// initialize v vector
		v = glm::vec3(verticesTab[indiceHoriz + 3] - verticesTab[indiceHoriz],
			verticesTab[indiceHoriz + 4] - verticesTab[indiceHoriz + 1],
			verticesTab[indiceHoriz + 5] - verticesTab[indiceHoriz + 2]);
		v = safeNormalize(v);

		// set the normal in the face normals array
		faceNormals[j] = glm::cross(u, v);

		// second triangle
		// initialize u vector
		u = glm::vec3(verticesTab[indiceHoriz + 3] - verticesTab[indiceVer + 3],
			verticesTab[indiceHoriz + 4] - verticesTab[indiceVer + 4],
			verticesTab[indiceHoriz + 5] - verticesTab[indiceVer + 5]);
		u = safeNormalize(u);

		// initialize v vector
		v = glm::vec3(verticesTab[indiceVer] - verticesTab[indiceVer + 3],
			verticesTab[indiceVer + 1] - verticesTab[indiceVer + 4],
			verticesTab[indiceVer + 2] - verticesTab[indiceVer + 5]);
		v = safeNormalize(v);

		// set the normal in the face normals array
		faceNormals[j + 1] = glm::cross(u, v);
			i += 6;
			j += 2;
		}
	}

	// Vertex normals: one indexed pass over the triangle list. Each
	// vertex accumulates the face normals of its adjacent triangles
	// and normalizes the sum. This replaces the firstLine /
	// middleGrid / lastLine / firstVertex bookkeeping, which averaged
	// the same adjacent faces without normalizing (and read faces from
	// the wrong row for the first vertex of the last line).
	accumulateVertexNormals(indicesTab.data(),
		(this->numberVerticesX - 1) * (this->numberVerticesZ - 1) * 6,
		faceNormals.data(), this->numberVerticesX * this->numberVerticesZ,
		normalsF.data());

	// set the arrays in the Geometry
	this->geometry = std::make_unique<Geometry>(
		verticesTab.data(), indicesTab.data(),
		this->numberVerticesX * this->numberVerticesZ * 3,
		(this->numberVerticesX - 1) * (this->numberVerticesZ - 1) * 6);

	this->geometry->setNormals(normalsF.data());
	mat->setHeightRange(MAX_HEIGHT);
	this->material = std::move(mat);
}

// Add every triangle's face normal to its three vertices, then store
// the normalized sums. Every adjacent face counts exactly once, so the
// direction matches the old per-line averaging; only the magnitude
// changes (unit instead of the raw average).
void Ground::accumulateVertexNormals(const int* indices, int indexCount,
	const glm::vec3* faceNormals, int vertexCount, float* out) {
	std::vector<glm::vec3> acc(static_cast<size_t>(vertexCount),
		glm::vec3(0.0f));
	const int triCount = indexCount / 3;
	for (int t = 0; t < triCount; ++t) {
		const glm::vec3& f = faceNormals[t];
		acc[static_cast<size_t>(indices[t * 3])] += f;
		acc[static_cast<size_t>(indices[t * 3 + 1])] += f;
		acc[static_cast<size_t>(indices[t * 3 + 2])] += f;
	}
	for (int v = 0; v < vertexCount; ++v) {
		const glm::vec3 n = safeNormalize(acc[static_cast<size_t>(v)]);
		out[v * 3] = n.x;
		out[v * 3 + 1] = n.y;
		out[v * 3 + 2] = n.z;
	}
}

// Height of the plane (p, n) above (x, z). Fails fast instead of
// dividing by a vertical normal or returning a magic fallback height.
float Ground::planeHeightAt(const glm::vec3& p, const glm::vec3& n,
	float x, float z) {
	if (x <= -MAP_SIZE / 2 || x >= MAP_SIZE / 2 || z <= -MAP_SIZE / 2 ||
		z >= MAP_SIZE / 2)
		throw std::out_of_range("Ground::getY coordinates off the map");
	if (std::fabs(n.y) <= std::numeric_limits<float>::epsilon())
		throw std::runtime_error("Ground::getY vertical face normal");
	// constant of the plane equation
	double d = -(p.x * n.x + p.y * n.y + p.z * n.z);
	// compute and return the interpolation
	return static_cast<float>(-(d + n.z * z + n.x * x) / n.y);
}

// return the y value depenting of the x and z values
float Ground::getY(float px, float pz) {
	// check if the camera inside the ground square
	if (px < MAP_SIZE / 2 && px > -MAP_SIZE / 2 && pz < MAP_SIZE / 2 &&
		pz > -MAP_SIZE / 2) {
		int indx;
		int indz;
		glm::vec3 p, n;

		// compute the nearest known vertex
		indx = static_cast<int>(trunc((px + MAP_SIZE / 2) * (this->numberVerticesX - 1) / MAP_SIZE));
		indz = static_cast<int>(trunc((pz + MAP_SIZE / 2) * (this->numberVerticesZ - 1) / MAP_SIZE));
		// Clamp to quads: the last row/column has no face below/right of
		// it, and float rounding at the border could step past the end.
		if (indx < 0) indx = 0;
		if (indz < 0) indz = 0;
		if (indx > numberVerticesX - 2) indx = numberVerticesX - 2;
		if (indz > numberVerticesZ - 2) indz = numberVerticesZ - 2;

		// point of the plane use to compute the constant value
		p = glm::vec3(this->geometry
			->getVertices()[(indx + indz * this->numberVerticesX) * 3],
			this->geometry
			->getVertices()[(indx + indz * this->numberVerticesX) * 3 + 1],
			this->geometry
			->getVertices()[(indx + indz * this->numberVerticesX) * 3 + 2]);

		// get the normal of the current plane to construct the plane equation
		n = this->faceNormals[(this->numberVerticesX - 1) * 2 * indz + indx * 2];
		return planeHeightAt(p, n, px, pz);
	}
	throw std::out_of_range("Ground::getY coordinates off the map");
}

// compute the approximative value of y, based on the four planes around and the
// previous Y coordinate
float Ground::getYApprox(float x, float z, float prevY) {
  // Neighbor taps step one texel; at the rim that step leaves the map.
  // Clamp every tap inside the plane bounds so border pixels average
  // against the edge instead of throwing out_of_range.
  const float half = MAP_SIZE / 2.0f;
  const float lo = -half + 1e-3f;
  const float hi = half - 1e-3f;
  const float dx = MAP_SIZE / static_cast<float>(this->numberVerticesX - 1);
  const float dz = MAP_SIZE / static_cast<float>(this->numberVerticesZ - 1);
  const float xp = std::clamp(x + dx, lo, hi);
  const float xm = std::clamp(x - dx, lo, hi);
  const float zp = std::clamp(z + dz, lo, hi);
  const float zm = std::clamp(z - dz, lo, hi);
  return (prevY +
    (getY(x, z) + getY(xp, z) + getY(xm, z) + getY(x, zp) +
      getY(x, zm)) /
    5) /
    2;
}

std::optional<glm::vec3> Ground::randomMapPos(double height, bool isRock) {
	for (int samples = 0; samples < 20; ++samples) {
		const double x = Utils::random(mapMin, mapMax);
		const double z = Utils::random(mapMin, mapMax);
		if (!this->path->validate(x, z, mapMin, mapMax))
			continue;
		if (!isRock && this->containInBoundingBox(x, z))
			continue;

		if (isRock)
			rockPos.push_back(glm::vec3(static_cast<float>(x), 0, static_cast<float>(z)));

		const double y =
			getY(static_cast<float>(x), static_cast<float>(z)) + height;
		return glm::vec3(static_cast<float>(x), static_cast<float>(y),
			static_cast<float>(z));
	}

	return std::nullopt;
}

bool Ground::containInBoundingBox(double x, double z) {

	for (const glm::vec3& rock : this->rockPos) {
		if ((rock.x - x) * (rock.x - x) + (rock.z - z) * (rock.z - z) < kRockExclusionRadiusSq) {
			return true;
		}
	}

	return false;
}

std::vector<glm::vec3> Ground::getRockPos() { return this->rockPos; }

void Ground::removeRockPos() {
	rockPos.clear();
}

void Ground::setMapBounds(float minV, float maxV) {
	mapMin = minV;
	mapMax = maxV;
}

float Ground::getMapMin() const { return mapMin; }

float Ground::getMapMax() const { return mapMax; }

Ground::~Ground()
{
}
