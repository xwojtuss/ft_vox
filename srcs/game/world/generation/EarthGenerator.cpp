#include "EarthGenerator.hpp"
#include <random>
#include "../../../scene/WorldInfo.hpp"

#include <algorithm>

using namespace game::world;

EarthGenerator::EarthGenerator() : m_heightMap(16, 16, 4, 0.01f) {
}

// TODO: refactor to split this mess
void EarthGenerator::generateChunk(Chunk* chunk, const glm::ivec3 chunkPosition) const {
	const int     worldChunkBaseY  = static_cast<int>(chunkPosition.y) * chunkYSize;
	constexpr int maxTerrainHeight = std::min<int>(scene::worldinfo::terrainMaxHeightBlocks,
													chunkYSize * scene::worldinfo::maxVerticalRenderDistance);
	int worldX        = 0;
	int worldY        = 0;
	int worldZ        = 0;
	int terrainHeight = 0;

	for (unsigned short blockX = 0; blockX < chunkXSize; ++blockX) {
		for (unsigned short blockZ = 0; blockZ < chunkZSize; ++blockZ) {
			worldX = static_cast<int>(chunkPosition.x) * chunkXSize + static_cast<int>(blockX);
			worldZ = static_cast<int>(chunkPosition.z) * chunkZSize + static_cast<int>(blockZ);

			terrainHeight = static_cast<int>(m_heightMap.getNormalizedValue(worldX, worldZ) * maxTerrainHeight);

			for (unsigned short blockY = 0; blockY < chunkYSize; ++blockY) {
				worldY = worldChunkBaseY + blockY;

				if (worldY >= terrainHeight)
					continue;

				chunk->setBlock(blockX, blockY, blockZ, game::Block(1));
			}
		}
	}
}
