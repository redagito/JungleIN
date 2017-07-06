#include "objloader.h"

#include <iostream>

#include <tiny_obj_loader.h>

#include "materials/materialbasic.h"

// http://www.limegarden.net/2010/03/02/wavefront-obj-mesh-loader/

struct MeshFace {
  int pos_index[3];
  int tex_index[3];
  int nor_index[3];
};

Mesh *ObjLoader::loadObj(std::string fileName) {
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string err;
	if (!tinyobj::LoadObj(shapes, materials, err, fileName.c_str()))
	{
		std::cout << err << std::endl;
		return nullptr;
	}
  Geometry *geometry = new Geometry(shapes.at(0).mesh.positions, shapes.at(0).mesh.indices);
  geometry->setNormals(shapes.at(0).mesh.normals);
  geometry->setTexCoords(shapes.at(0).mesh.texcoords);

  Mesh *mesh = new Mesh(geometry, new MaterialBasic(Vector3(0.4f, 0.6f, 0.2f)));

  return mesh;
}
