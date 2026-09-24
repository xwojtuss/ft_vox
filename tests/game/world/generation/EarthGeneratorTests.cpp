#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include <cstdlib>
#include <map>

#include "game/world/generation/EarthGenerator.hpp"
#include "support/Blocks.hpp"

using game::world::Chunk;
using game::world::EarthGenerator;
using game::world::chunkXSize;
using game::world::chunkYSize;
using game::world::chunkZSize;

namespace {
Chunk	generate(glm::ivec3 chunkPosition) {
	EarthGenerator	generator;
	Chunk			chunk;

	generator.generateChunk(&chunk, chunkPosition);
	return chunk;
}

size_t	countBlocks(const Chunk& chunk, game::BlockId id) {
	size_t count = 0;
	for (unsigned short x = 0; x < chunkXSize; ++x)
		for (unsigned short y = 0; y < chunkYSize; ++y)
			for (unsigned short z = 0; z < chunkZSize; ++z)
				count += chunk.getBlock(x, y, z).id == id;
	return count;
}

/**
 * Stacks the chunks of one vertical column and returns, for a block column,
 * the world heights that are solid
 */
std::map<int, bool>	solidByWorldHeight(const std::map<int, Chunk>& stack, unsigned short x, unsigned short z) {
	std::map<int, bool> solid;
	for (const auto& [chunkY, chunk] : stack)
		for (unsigned short y = 0; y < chunkYSize; ++y)
			solid[chunkY * chunkYSize + y] = chunk.getBlock(x, y, z).id != test::air;
	return solid;
}

int	surfaceHeight(const Chunk& groundChunk, unsigned short x, unsigned short z) {
	int height = 0;
	while (height < chunkYSize && groundChunk.getBlock(x, height, z).id != test::air)
		++height;
	return height;
}
}

SCENARIO("The same chunk is always generated the same way", "[generation]") {
	GIVEN("the chunk at (2, 0, -3) generated twice by separate generators") {
		const Chunk first = generate({2, 0, -3});
		const Chunk second = generate({2, 0, -3});

		THEN("both contain exactly the same blocks") {
			for (unsigned short x = 0; x < chunkXSize; ++x)
				for (unsigned short y = 0; y < chunkYSize; ++y)
					for (unsigned short z = 0; z < chunkZSize; ++z)
						REQUIRE(first.getBlock(x, y, z).id == second.getBlock(x, y, z).id);
		}
	}
}

SCENARIO("Generated terrain is made of dirt and air only", "[generation]") {
	GIVEN("a chunk at ground level") {
		const Chunk chunk = generate({0, 0, 0});

		THEN("every block is either dirt or air") {
			REQUIRE(countBlocks(chunk, test::dirt) + countBlocks(chunk, test::air)
				== static_cast<size_t>(chunkXSize * chunkYSize * chunkZSize));
		}
	}
}

SCENARIO("Everything below the world's floor is solid", "[generation]") {
	GIVEN("a chunk below y = 0, at a negative horizontal position") {
		const Chunk chunk = generate({-4, -1, -7});

		THEN("it is completely filled with dirt") {
			REQUIRE(countBlocks(chunk, test::dirt) == static_cast<size_t>(chunkXSize * chunkYSize * chunkZSize));
		}
	}
}

SCENARIO("Nothing is generated above the maximum terrain height", "[generation]") {
	GIVEN("the first chunk that starts at the maximum terrain height (64 blocks)") {
		const int	chunkY = scene::worldinfo::terrainMaxHeightBlocks / chunkYSize;
		const Chunk	chunk = generate({1, chunkY, 1});

		THEN("it is empty") {
			REQUIRE(countBlocks(chunk, test::air) == static_cast<size_t>(chunkXSize * chunkYSize * chunkZSize));
		}
	}
}

SCENARIO("Terrain has no floating blocks or caves", "[generation]") {
	GIVEN("a full vertical stack of chunks from below the floor to above the maximum height") {
		const glm::ivec2	column = GENERATE(glm::ivec2(0, 0), glm::ivec2(-3, 5), glm::ivec2(12, -9));
		std::map<int, Chunk> stack;

		for (int chunkY = -1; chunkY <= 4; ++chunkY)
			stack.emplace(chunkY, generate({column.x, chunkY, column.y}));

		THEN("in every block column, all blocks below the surface are solid and all above are air") {
			for (unsigned short x = 0; x < chunkXSize; ++x) {
				for (unsigned short z = 0; z < chunkZSize; ++z) {
					bool reachedSurface = false;
					for (const auto& [worldY, solid] : solidByWorldHeight(stack, x, z)) {
						if (!solid)
							reachedSurface = true;
						else
							REQUIRE_FALSE(reachedSurface);
					}
				}
			}
		}
	}
}

SCENARIO("Terrain continues smoothly across chunk borders", "[generation][regression]") {
	GIVEN("two neighbouring ground chunks on either side of x = 0") {
		const Chunk west = generate({-1, 0, 0});
		const Chunk east = generate({0, 0, 0});

		THEN("the surface steps by at most 2 blocks between the touching columns") {
			for (unsigned short z = 0; z < chunkZSize; ++z)
				REQUIRE(std::abs(surfaceHeight(west, chunkXSize - 1, z) - surfaceHeight(east, 0, z)) <= 2);
		}
	}

	GIVEN("two neighbouring ground chunks on either side of z = 0") {
		const Chunk north = generate({3, 0, -1});
		const Chunk south = generate({3, 0, 0});

		THEN("the surface steps by at most 2 blocks between the touching columns") {
			for (unsigned short x = 0; x < chunkXSize; ++x)
				REQUIRE(std::abs(surfaceHeight(north, x, chunkZSize - 1) - surfaceHeight(south, x, 0)) <= 2);
		}
	}
}
