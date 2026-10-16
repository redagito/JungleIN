#include "helpers/ObjLoader.h"

#include <tiny_obj_loader.h>
#include "helpers/Logger.h"
#include <stdexcept>

#include "materials/MaterialBasic.h"

// http://www.limegarden.net/2010/03/02/wavefront-obj-mesh-loader/

Mesh* ObjLoader::loadObj(std::string fileName) 
{
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials; // Ignored
	std::string warn;
	std::string err;
	auto result = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, fileName.c_str());

	if (!warn.empty()) logw("{}", warn);
	// Single contract: any loader error throws. An error string without a
	// failure flag is still an error, never a downgrade to a warning.
	if (!result || !err.empty())
		throw std::runtime_error("Failed to load obj from file: " + fileName +
			(err.empty() ? "" : " (" + err + ")"));

	// tinyobj produces separate indices for vertices, normals and uvs which we need to "fix"
	FlattenedMesh flat = flatten(attrib, shapes, fileName);
	// A parse that yields no geometry is an asset bug: fail here instead
	// of handing a hollow Mesh to the GL pipeline.
	if (flat.vertices.empty())
		throw std::runtime_error(
			"OBJ file contains no geometry: " + fileName);

	Geometry* geometry = new Geometry(flat.vertices, flat.indices);
	geometry->setNormals(flat.normals);
	geometry->setTexCoords(flat.uvs);

	Mesh* mesh = new Mesh(geometry, new MaterialBasic(glm::vec3(0.4f, 0.6f, 0.2f)));

	return mesh;
}

ObjLoader::FlattenedMesh ObjLoader::flatten(
	const tinyobj::attrib_t& attrib,
	const std::vector<tinyobj::shape_t>& shapes, const std::string& source) {
	FlattenedMesh flat;
	// Flatten and create "dummy" index buffer
	unsigned int currentIndex = 0;
	for (size_t s = 0; s < shapes.size(); ++s) {
		const auto& indices = shapes[s].mesh.indices;
		for (size_t f = 0; f < indices.size(); ++f) {
			const auto& index = indices[f];
			const std::string where =
				source + " shape " + std::to_string(s) + " face " + std::to_string(f);
			if (index.vertex_index < 0 ||
				static_cast<size_t>(index.vertex_index) * 3 + 2 >= attrib.vertices.size())
				throw std::runtime_error("OBJ vertex index out of range in " + where);
			// Models without normals are rejected by design; all shipped assets carry them.
		if (index.normal_index < 0 ||
				static_cast<size_t>(index.normal_index) * 3 + 2 >= attrib.normals.size())
				throw std::runtime_error("OBJ normal index out of range in " + where);
			// Vertex x/y/z
			flat.vertices.push_back(attrib.vertices.at(index.vertex_index * 3));
			flat.vertices.push_back(attrib.vertices.at(index.vertex_index * 3 + 1));
			flat.vertices.push_back(attrib.vertices.at(index.vertex_index * 3 + 2));
			// Normal x/y/z
			flat.normals.push_back(attrib.normals.at(index.normal_index * 3));
			flat.normals.push_back(attrib.normals.at(index.normal_index * 3 + 1));
			flat.normals.push_back(attrib.normals.at(index.normal_index * 3 + 2));
			// UVs
			if (index.texcoord_index >= 0)
			{
				if (static_cast<size_t>(index.texcoord_index) * 2 + 1 >= attrib.texcoords.size())
					throw std::runtime_error("OBJ texcoord index out of range in " + where);
				flat.uvs.push_back(attrib.texcoords.at(index.texcoord_index * 2));
				flat.uvs.push_back(attrib.texcoords.at(index.texcoord_index * 2 + 1));
			}
			else
			{
				flat.uvs.push_back(0.f);
				flat.uvs.push_back(0.f);
			}

			flat.indices.push_back(currentIndex);
			++currentIndex;
		}
	}
	return flat;
}
