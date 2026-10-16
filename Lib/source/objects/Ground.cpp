#include "helpers/Logger.h"

#include "objects/Ground.h"
#include <cmath>
#include <limits>
#include <stdexcept>

namespace {
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
float Ground::MAP_MIN = -120.0;
float Ground::MAP_MAX = 120.0;
const float Ground::MAX_HEIGHT = 10.0;

Ground::Ground(std::string filePath, MaterialGround* mat) : Mesh(NULL, NULL) {
	// temporary array initialized here, and then given to a Geometry
	float* verticesTab;
	int* indicesTab;
	float* normalsF;
	// used as array indice
	int i = 0;

	// load the heightmap picture (data map: no flip, QImage origin).
	// The Path mask is allocated only after this succeeds: if the image
	// load throws, there is no half-built Path left behind to leak (the
	// destructor never runs on a failed construction).
	Image img(filePath, false);
	this->path = new Path("data/resources/path/path.png");
	// init the size of the loaded picture
	this->numberVerticesX = img.getWidth();
	this->numberVerticesZ = img.getHeight();

	// init the array's sizes
	verticesTab = new float[this->numberVerticesX * this->numberVerticesZ * 3];
	// 6 indices for each quad (made with 2 triangles)
	indicesTab =
		new int[(this->numberVerticesX - 1) * (this->numberVerticesZ - 1) * 6];
	// 2 normals for each quad (made with 2 triangles)
	faceNormals = new glm::vec3[(this->numberVerticesX - 1) *
		(this->numberVerticesZ - 1) * 2];
	// there is 3 coordinates for each normal
	normalsF =
		new float[(this->numberVerticesX - 1) * (this->numberVerticesZ - 1) * 6];

	double poolEffect = 0.0;
	double poolIntensity = 20.0;
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

	// compute the normals of the first line of vertices
	firstLineNormals(normalsF);
	// compute the normals of all the lines of vertices except for the first and
	// the last lines
	middleGridNormals(normalsF);
	// compute the normals of the last line of vertices
	lastLineNormals(normalsF);

	// set the arrays in the Geometry
	this->geometry = new Geometry(
		verticesTab, indicesTab,
		this->numberVerticesX * this->numberVerticesZ * 3,
		(this->numberVerticesX - 1) * (this->numberVerticesZ - 1) * 6);

	this->geometry->setNormals(normalsF);
	mat->setHeightRange(MAX_HEIGHT);
	this->material = mat;

	// release the memory master please!
	delete[] normalsF;
	delete[] verticesTab;
	delete[] indicesTab;
}

// put the values of the normal vector into the i, i+1, i+2 th positions of the
// floats array
void Ground::setVectorToFloat(glm::vec3 normal, float* normalsF, int i) {
	normalsF[i] = normal.x;
	normalsF[i + 1] = normal.y;
	normalsF[i + 2] = normal.z;
}

// compute the normals of the first line of vertices
void Ground::firstLineNormals(float* normalsF) {
	int i, j;
	glm::vec3 tmp = faceNormals[0];
	// normal of the first vertex
	setVectorToFloat(tmp, normalsF, 0);

	// normal of the first line exept the last vertice
	for (i = 3, j = 0; i < (this->numberVerticesX - 1) * 3; i += 3, j += 2) {
		tmp = (faceNormals[j] + faceNormals[j + 1] + faceNormals[j + 2]) / 3.0f;
		setVectorToFloat(tmp, normalsF, i);
	}

	// normal of the last vertice of the first line
	tmp = (faceNormals[j] + faceNormals[j + 1]) / 2.0f;
	setVectorToFloat(tmp, normalsF, i);
}

// compute the normals of all the lines of vertices except for the first and the
// last
void Ground::middleGridNormals(float* normalsF) {
	int k = (this->numberVerticesX - 1) * 2;
	glm::vec3 tmp;
	for (int l = 1; l < this->numberVerticesZ - 1; ++l) {
		// first vertex of the line
		firstVertexNormal(normalsF, l);
		// all verices except the last
		for (int j = 1; j < k - 1; j += 2) {
			int normalIndex = l * k + j;
			tmp = (faceNormals[normalIndex - k] + faceNormals[normalIndex - k + 1] +
				faceNormals[normalIndex - k + 2] + faceNormals[normalIndex - 1] +
				faceNormals[normalIndex] + faceNormals[normalIndex + 1]) / 6.0f;
			setVectorToFloat(tmp, normalsF,
				(this->numberVerticesX * l + ((j + 1) / 2)) * 3);
		}
		// last vertex of the line
		int t = l * k - 1;
		tmp = (faceNormals[t] + faceNormals[t + k - 1] + faceNormals[t + k]) / 3.0f;
		setVectorToFloat(tmp, normalsF, (this->numberVerticesX * (l + 1) - 1) * 3);
	}
}

// compute the normals of the last line of vertices
void Ground::lastLineNormals(float* normalsF) {
	// indice of the first vertex of the last line
	int indice = this->numberVerticesX * (this->numberVerticesZ - 1);
	int l = this->numberVerticesZ - 2;
	int k = (this->numberVerticesX - 1) * 2;
	glm::vec3 tmp;

	// first vertex normal
	tmp = (faceNormals[indice + 1] + faceNormals[indice + 2]) / 2.0f;
	setVectorToFloat(tmp, normalsF, indice * 3);
	// all vertices except the last
	for (int i = 1, j = 0; i < this->numberVerticesX - 1; ++i, j += 2) {
		int indTmp = l * k + j;
		tmp = (faceNormals[indTmp + 1] + faceNormals[indTmp + 2] + faceNormals[indTmp + 3]) / 3.0f;
		setVectorToFloat(tmp, normalsF, (indice + i) * 3);
	}
	// last vertex of the last line. So, it's the last vertex of ALL the ground.
	// Yeah! It belongs to the second triangle of the last quad, whose face
	// normal is the last entry of the face array.
	setVectorToFloat(
		faceNormals[2 * (this->numberVerticesX - 1) * (this->numberVerticesZ - 1) - 1],
		normalsF, (this->numberVerticesX * this->numberVerticesZ - 1) * 3);
}

// compute the normal of the first vertex of the line (doesn't work for the
// first ans the last line)
void Ground::firstVertexNormal(float* normalsF, int lineNumber) {
	glm::vec3 tmp;
	int k = (this->numberVerticesX - 1) * 2;
	tmp = (faceNormals[lineNumber * k - k] + faceNormals[lineNumber * k - k + 1] +
		faceNormals[lineNumber * k]) / 3.0f;
	setVectorToFloat(tmp, normalsF, this->numberVerticesX * lineNumber * 3);
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
	return (prevY +
		(getY(x, z) + getY(x + MAP_SIZE / (this->numberVerticesX - 1), z) +
			getY(x - MAP_SIZE / (this->numberVerticesX - 1), z) +
			getY(x, z + MAP_SIZE / (this->numberVerticesZ - 1)) +
			getY(x, z - MAP_SIZE / (this->numberVerticesZ - 1))) /
		5) /
		2;
}

std::optional<glm::vec3> Ground::randomMapPos(double height, bool isRock) {
	for (int samples = 0; samples < 20; ++samples) {
		const double x = Utils::random(MAP_MIN, MAP_MAX);
		const double z = Utils::random(MAP_MIN, MAP_MAX);
		if (!this->path->validate(x, z))
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
		if ((rock.x - x) * (rock.x - x) + (rock.z - z) * (rock.z - z) < 40) {
			return true;
		}
	}

	return false;
}

std::vector<glm::vec3> Ground::getRockPos() { return this->rockPos; }

void Ground::removeRockPos() {
	rockPos.clear();
}

Ground::~Ground()
{
	delete[] this->faceNormals;
	delete this->path;
}
