#include "Resources.hpp"

using namespace assets;

uint64_t MeshHandle::nextId    = 0;
uint64_t TextureHandle::nextId = 0;

void MeshData::scaleTextureCoordinates(float scale) {
	for (auto& vertex: vertices) {
		vertex.texCoord *= scale;
	}
}
