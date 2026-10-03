#include <catch2/catch_test_macros.hpp>

#include "game/planet/ChunkLoader.hpp"
#include "support/Blocks.hpp"
#include "support/TestSeed.hpp"

using game::planet::Chunk;
using game::planet::ChunkLoader;
using game::planet::chunkXSize;
using game::planet::chunkYSize;
using game::planet::chunkZSize;
using game::planet::EarthGenerator;

SCENARIO("Loading a chunk generates its terrain", "[chunk-loader]") {
	GIVEN("a chunk loader") {
		const ChunkLoader loader(test::seed);

		WHEN("the chunk at (-2, 0, 5) is loaded") {
			const std::unique_ptr<Chunk> loaded = loader.loadChunk({-2, 0, 5});

			THEN("a chunk is returned") {
				REQUIRE(loaded != nullptr);
			}
			AND_THEN("it holds the same terrain the terrain generator produces for that position") {
				const EarthGenerator generator(test::seed);
				Chunk                expected;
				generator.generateChunk(&expected, {-2, 0, 5});

				for (int x = 0; x < chunkXSize; ++x)
					for (int y = 0; y < chunkYSize; ++y)
						for (int z = 0; z < chunkZSize; ++z)
							REQUIRE(loaded->getBlock(x, y, z).id == expected.getBlock(x, y, z).id);
			}
		}

		WHEN("the same chunk is loaded twice") {
			const std::unique_ptr<Chunk> first  = loader.loadChunk({0, 0, 0});
			const std::unique_ptr<Chunk> second = loader.loadChunk({0, 0, 0});

			THEN("each load returns its own copy") {
				REQUIRE(first.get() != second.get());
			}
		}
	}
}
