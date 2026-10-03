#include <catch2/catch_test_macros.hpp>

#include <utility>
#include <catch2/generators/catch_generators.hpp>

#include <cstdlib>
#include <map>

#include "scene/PlanetInfo.hpp"
#include "game/planet/generation/EarthGenerator.hpp"
#include "support/Blocks.hpp"
#include "support/TestSeed.hpp"

using game::planet::Chunk;
using game::planet::chunkXSize;
using game::planet::chunkYSize;
using game::planet::chunkZSize;
using game::planet::EarthGenerator;

namespace {
	Chunk generate(glm::ivec3 chunkPosition) {
		const EarthGenerator generator(test::seed);
		Chunk                chunk;

		generator.generateChunk(&chunk, chunkPosition);
		return chunk;
	}

	size_t countBlocks(const Chunk& chunk, game::BlockId id) {
		size_t count = 0;
		for (int x = 0; x < chunkXSize; ++x)
			for (int y = 0; y < chunkYSize; ++y)
				for (int z = 0; z < chunkZSize; ++z)
					count += static_cast<size_t>(chunk.getBlock(x, y, z).id == id);
		return count;
	}

	std::map<int, bool> solidByPlanetHeight(const std::map<int, Chunk>& stack, int x, int z) {
		std::map<int, bool> solid;
		for (const auto& [chunkY, chunk]: stack)
			for (int y = 0; y < chunkYSize; ++y)
				solid[(chunkY * chunkYSize) + y] = chunk.getBlock(x, y, z).id != test::air;
		return solid;
	}

	int surfaceHeight(const Chunk& groundChunk, int x, int z) {
		int height = 0;
		while (std::cmp_less(height, chunkYSize) && groundChunk.getBlock(x, height, z).id != test::air)
			++height;
		return height;
	}
}

SCENARIO("The same chunk is always generated the same way", "[generation]") {
	GIVEN("the chunk at (2, 0, -3) generated twice by separate generators") {
		const Chunk first  = generate({2, 0, -3});
		const Chunk second = generate({2, 0, -3});

		THEN("both contain exactly the same blocks") {
			for (int x = 0; x < chunkXSize; ++x)
				for (int y = 0; y < chunkYSize; ++y)
					for (int z = 0; z < chunkZSize; ++z)
						REQUIRE(first.getBlock(x, y, z).id == second.getBlock(x, y, z).id);
		}
	}
}

SCENARIO("Generated terrain is made of dirt and air only", "[generation]") {
	GIVEN("a chunk at ground level") {
		const Chunk chunk = generate({0, 0, 0});

		THEN("every block is either dirt or air") {
			REQUIRE(countBlocks(chunk, test::dirt) + countBlocks(chunk, test::air) ==
					static_cast<size_t>(chunkXSize * chunkYSize * chunkZSize));
		}
	}
}

SCENARIO("Everything below the planet's floor is solid", "[generation]") {
	GIVEN("a chunk below y = 0, at a negative horizontal position") {
		const Chunk chunk = generate({-4, -1, -7});

		THEN("it is completely filled with dirt") {
			REQUIRE(countBlocks(chunk, test::dirt) == static_cast<size_t>(chunkXSize * chunkYSize * chunkZSize));
		}
	}
}

SCENARIO("Nothing is generated above the maximum terrain height", "[generation]") {
	GIVEN("the first chunk that starts at the maximum terrain height (64 blocks)") {
		const int   chunkY = scene::planetinfo::terrainMaxHeightBlocks / chunkYSize;
		const Chunk chunk  = generate({1, chunkY, 1});

		THEN("it is empty") {
			REQUIRE(countBlocks(chunk, test::air) == static_cast<size_t>(chunkXSize * chunkYSize * chunkZSize));
		}
	}
}

SCENARIO("Terrain has no floating blocks or caves", "[generation]") {
	GIVEN("a full vertical stack of chunks from below the floor to above the maximum height") {
		const glm::ivec2     column = GENERATE(glm::ivec2(0, 0), glm::ivec2(-3, 5), glm::ivec2(12, -9));
		std::map<int, Chunk> stack;

		for (int chunkY = -1; chunkY <= 4; ++chunkY)
			stack.emplace(chunkY, generate({column.x, chunkY, column.y}));

		THEN("in every block column, all blocks below the surface are solid and all above are air") {
			for (int x = 0; x < chunkXSize; ++x) {
				for (int z = 0; z < chunkZSize; ++z) {
					bool reachedSurface = false;
					for (const auto& [planetY, solid]: solidByPlanetHeight(stack, x, z)) {
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
			for (int z = 0; z < chunkZSize; ++z)
				REQUIRE(std::abs(surfaceHeight(west, chunkXSize - 1, z) - surfaceHeight(east, 0, z)) <= 2);
		}
	}

	GIVEN("two neighbouring ground chunks on either side of z = 0") {
		const Chunk north = generate({3, 0, -1});
		const Chunk south = generate({3, 0, 0});

		THEN("the surface steps by at most 2 blocks between the touching columns") {
			for (int x = 0; x < chunkXSize; ++x)
				REQUIRE(std::abs(surfaceHeight(north, x, chunkZSize - 1) - surfaceHeight(south, x, 0)) <= 2);
		}
	}
}

SCENARIO("The seed decides the terrain", "[generation][seed]") {
	GIVEN("two generators with the same seed") {
		const EarthGenerator first(game::Seed(77));
		const EarthGenerator second(game::Seed(77));

		THEN("they generate the same chunk") {
			Chunk a;
			Chunk b;
			first.generateChunk(&a, {5, 0, -2});
			second.generateChunk(&b, {5, 0, -2});

			for (int x = 0; x < chunkXSize; ++x)
				for (int y = 0; y < chunkYSize; ++y)
					for (int z = 0; z < chunkZSize; ++z)
						REQUIRE(a.getBlock(x, y, z).id == b.getBlock(x, y, z).id);
		}
	}

	GIVEN("generators with different seeds") {
		const EarthGenerator first(game::Seed(1));
		const EarthGenerator second(game::Seed(2));

		THEN("their terrain differs somewhere in the area around the origin") {
			bool different = false;
			for (int chunkX = -3; chunkX <= 3 && !different; ++chunkX) {
				for (int chunkY = 0; chunkY <= 3 && !different; ++chunkY) {
					for (int chunkZ = -3; chunkZ <= 3 && !different; ++chunkZ) {
						Chunk a;
						Chunk b;
						first.generateChunk(&a, {chunkX, chunkY, chunkZ});
						second.generateChunk(&b, {chunkX, chunkY, chunkZ});
						different = countBlocks(a, test::dirt) != countBlocks(b, test::dirt);
					}
				}
			}
			REQUIRE(different);
		}
	}
}
