#pragma once

#include <array>
#include <fstream>
#include <sstream>
#include <random>
#include <string>
#include <functional>
#include <unordered_map>
#include <stdexcept>

#include "IModelLoader.hpp"
#include "Resources.hpp"
#include "../render/GpuTypes.hpp"
#include "../scene/WorldInfo.hpp"
#include "../platform/filesystem/resolvePath.hpp"

namespace assets {
struct FaceIndex {
	uint32_t vertexIndex;
	uint32_t texCoordIndex;
	uint32_t normalIndex;
};
class BasicObjLoader : public IModelLoader {
private:
	std::vector<glm::vec3>																	m_vertices;
	std::vector<glm::vec3>																	m_normals;
	std::vector<glm::vec2>																	m_textureCoordinates;
	std::vector<uint32_t>																	m_indices;
	std::vector<std::array<FaceIndex, 3>>													m_faces;
	std::unordered_map<std::string, std::function<void(std::stringstream&)>>				m_loaders;

	void		loadVertex(std::stringstream& sstream);
	void		loadFace(std::stringstream& sstream);
	void		loadTextureCoordinates(std::stringstream& sstream);
	void		loadVertexNormals(std::stringstream& sstream);
	void		parseIndices(std::stringstream& sstream, FaceIndex& faceIndex);
	void		createMeshData(MeshData& meshData);
	
	template <int N, typename T>
	void		parse(std::stringstream& sstream, glm::vec<N, T>& vec, int requiredCount = N);
	
	static void			skip(std::stringstream& sstream);
	static glm::vec2	mapUV(const glm::vec3& pos, const glm::vec3& normal, float scale);
	static void			changeOrigin(MeshData& meshData, glm::vec3 origin);

public:
	BasicObjLoader();
	virtual ~BasicObjLoader();

	MeshData	toMeshData(const char* path) override;
	MeshData	toMeshData(std::string path) override;
};
}

#include "BasicObjLoader.tpp"
