#include "TinyObjLoader.hpp"
#include <stdexcept>
#include "../render/GpuTypes.hpp"
#include "../platform/filesystem/resolvePath.hpp"

#include <unordered_map>

#include "../error/Exception.hpp"

using namespace assets;

MeshData TinyObjLoader::toMeshData(const char* path) {
	std::vector<render::Vertex>      vertices;
	std::vector<uint32_t>            indices;
	tinyobj::attrib_t                attrib;
	std::vector<tinyobj::shape_t>    shapes;
	std::vector<tinyobj::material_t> materials;
	std::string                      warn;

	if (std::string err; !tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, resolvePath(path).c_str())) {
		throw error::AssetError(path, err.empty()
										? "could not be loaded"
										: err.substr(0, err.find_last_not_of('\n') + 1));
	}

	std::unordered_map<render::Vertex, uint32_t> uniqueVertices{};

	for (const auto& shape: shapes) {
		for (const auto& index: shape.mesh.indices) {
			render::Vertex vertex{};

			vertex.pos = {
				attrib.vertices[3 * index.vertex_index + 0],
				attrib.vertices[3 * index.vertex_index + 1],
				attrib.vertices[3 * index.vertex_index + 2]
			};

			vertex.texCoord = {
				attrib.texcoords[2 * index.texcoord_index + 0],
				1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
			};

			vertex.color = {1.0f, 1.0f, 1.0f};
			if (uniqueVertices.count(vertex) == 0) {
				uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
				vertices.push_back(vertex);
			}

			indices.push_back(uniqueVertices[vertex]);
		}
	}

	return MeshData{vertices, indices};
}

MeshData TinyObjLoader::toMeshData(const std::string& path) {
	return toMeshData(path.c_str());
}
