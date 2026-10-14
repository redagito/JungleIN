#include "helpers/Geometries.h"

#include <glad/glad.h>

/*
    4_______7
   /|      /|
  / |     / |
0/__|____3  |
|   |____|__|
|  /5    |  /6
| /      | /
|________|/
1        2
*/
Geometry* Geometries::cube(float size) {
	float vertices[] = {
		-size, size,  size, // 0
		-size, -size, size, // 1
		size, -size, size, // 2
		size, size, size, // 3
		-size, size, -size, // 4
		-size, -size, -size, // 5
		size, -size, -size, // 6
		size, size,  -size, // 7
	};

	int indices[] = {
		0, 1, 2, 0, 2, 3, 
		3, 2, 6, 3, 6, 7, 
		7, 6, 5, 7, 5, 4, 
		4, 5, 1, 4, 1, 0,
		0, 3, 7, 0, 7, 4, 
		1, 2, 6, 1, 6, 5
	};

	Geometry* g = new Geometry(vertices, indices, 24, 36);
	g->setPrimitive(GL_TRIANGLES);

	return g;
}

// 0    |    3
//      |
// -----|-----
//      |
// 1    |    2
// 
// Winding order counter clockwise
// 
Geometry* Geometries::quad(float size) {
	float vertices[] = {
		-size, size,  0.0, // 0
		-size, -size, 0.0, // 1 
		size,  -size, 0.0, // 2
		size,  size,  0.0  // 3
	};

	float texcoords[] = { 0.0, 1.0, 0.0, 0.0, 1.0, 0.0, 1.0, 1.0 };

	int indices[] = {
		// First triangle
		0, 1, 3,
		// Second triangle
		1, 2, 3
	};

	Geometry* g = new Geometry(vertices, indices, 12, 6);
	g->setTexCoords(texcoords);
	g->setPrimitive(GL_TRIANGLES);

	return g;
}

Geometry* Geometries::point() {
	float tab_vertices[] = { 0, 0, 0 };
	int tab_indices[] = { 0 };

	Geometry* g = new Geometry(tab_vertices, tab_indices, 3, 1);

	return g;
}
