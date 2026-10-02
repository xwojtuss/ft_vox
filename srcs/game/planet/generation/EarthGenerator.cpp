#include "game/planet/generation/EarthGenerator.hpp"
#include "scene/PlanetInfo.hpp"

#include <algorithm>

using namespace game::planet;

EarthGenerator::EarthGenerator() : m_heightMap(16, 16, 4, 0.01f) {
}

// TODO: refactor to split this mess
void EarthGenerator::generateChunk(Chunk* chunk, const glm::ivec3 chunkPosition) const {
	const int     planetChunkBaseY = static_cast<int>(chunkPosition.y) * chunkYSize;
	constexpr int maxTerrainHeight = std::min<int>(scene::planetinfo::terrainMaxHeightBlocks,
												   chunkYSize * scene::planetinfo::maxVerticalRenderDistance);
	if (planetChunkBaseY >= maxTerrainHeight)
		return;

	int planetX       = 0;
	int planetY       = 0;
	int planetZ       = 0;
	int terrainHeight = 0;

	for (int blockX = 0; blockX < chunkXSize; ++blockX) {
		for (int blockZ = 0; blockZ < chunkZSize; ++blockZ) {
			planetX = (static_cast<int>(chunkPosition.x) * chunkXSize) + blockX;
			planetZ = (static_cast<int>(chunkPosition.z) * chunkZSize) + blockZ;

			terrainHeight = static_cast<int>(m_heightMap.getNormalizedValue(planetX, planetZ) * maxTerrainHeight);

			for (int blockY = 0; blockY < chunkYSize; ++blockY) {
				planetY = planetChunkBaseY + blockY;

				if (planetY >= terrainHeight)
					continue;

				chunk->setBlock(blockX, blockY, blockZ, game::Block(1));
			}
		}
	}
}
