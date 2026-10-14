#include "helpers/ObjLoader.h"

#include <tiny_obj_loader.h>
#include <fmtlog/fmtlog.h>
#include <stdexcept>

#include "materials/MaterialBasic.h"

// http://www.limegarden.net/2010/03/02/wavefront-obj-mesh-loader/

struct MeshFace {
	int pos_index[3];
	int tex_index[3];
	int nor_index[3];
};

Mesh* ObjLoader::loadObj(std::string fileName) 
{
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials; // Ignored
	std::string warn;
	std::string err;
	auto result = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, fileName.c_str());

	if (!warn.empty()) logw("{}", warn);
	if (!err.empty()) logw("{}", err);
	if (!result) throw std::runtime_error("Failed to load obj");

	// tinyobj produces separate indices for vertices, normals and uvs which we need to "fix"
	std::vector<float> vertices;
	std::vector<float> normals;
	std::vector<float> uvs;
	std::vector<unsigned int> indices;
    // Flatten and create "dummy" index buffer
    unsigned int currentIndex = 0;
    for (auto shape : shapes)
    {
        for (auto index : shape.mesh.indices)
        {
            // Vertex x/y/z
            vertices.push_back(attrib.vertices.at(index.vertex_index * 3));
            vertices.push_back(attrib.vertices.at(index.vertex_index * 3 + 1));
            vertices.push_back(attrib.vertices.at(index.vertex_index * 3 + 2));
            // Normal x/y/z
            normals.push_back(attrib.normals.at(index.normal_index * 3));
            normals.push_back(attrib.normals.at(index.normal_index * 3 + 1));
            normals.push_back(attrib.normals.at(index.normal_index * 3 + 2));
            // UVs
            if (index.texcoord_index >= 0)
            {
                uvs.push_back(attrib.texcoords.at(index.texcoord_index * 2));
                uvs.push_back(attrib.texcoords.at(index.texcoord_index * 2 + 1));
            }
            else
            {
                uvs.push_back(0.f);
                uvs.push_back(0.f);
            }

            indices.push_back(currentIndex);
            ++currentIndex;
        }
    }

	Geometry* geometry = new Geometry(vertices, indices);
	geometry->setNormals(normals);
	geometry->setTexCoords(uvs);

	Mesh* mesh = new Mesh(geometry, new MaterialBasic(Vector3(0.4f, 0.6f, 0.2f)));

	return mesh;
}
