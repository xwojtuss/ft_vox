#include <catch2/catch_test_macros.hpp>

#include "game/world/ChunkLoader.hpp"
#include "support/Blocks.hpp"

using game::world::Chunk;
using game::world::ChunkLoader;
using game::world::EarthGenerator;
using game::world::chunkXSize;
using game::world::chunkYSize;
using game::world::chunkZSize;

SCENARIO("Loading a chunk generates its terrain", "[chunk-loader]") {
	GIVEN("a chunk loader") {
		ChunkLoader loader;

		WHEN("the chunk at (-2, 0, 5) is loaded") {
			const std::unique_ptr<Chunk> loaded = loader.loadChunk({-2, 0, 5});

			THEN("a chunk is returned") {
				REQUIRE(loaded != nullptr);
			}
			AND_THEN("it holds the same terrain the world generator produces for that position") {
				EarthGenerator	generator;
				Chunk			expected;
				generator.generateChunk(&expected, {-2, 0, 5});

				for (unsigned short x = 0; x < chunkXSize; ++x)
					for (unsigned short y = 0; y < chunkYSize; ++y)
						for (unsigned short z = 0; z < chunkZSize; ++z)
							REQUIRE(loaded->getBlock(x, y, z).id == expected.getBlock(x, y, z).id);
			}
		}

		WHEN("the same chunk is loaded twice") {
			const std::unique_ptr<Chunk> first = loader.loadChunk({0, 0, 0});
			const std::unique_ptr<Chunk> second = loader.loadChunk({0, 0, 0});

			THEN("each load returns its own copy") {
				REQUIRE(first.get() != second.get());
			}
		}
	}
}

SCENARIO("Saving a chunk is accepted but not implemented yet", "[chunk-loader]") {
	GIVEN("a chunk loader") {
		ChunkLoader loader;

		THEN("saving any chunk does not fail") {
			REQUIRE_NOTHROW(loader.saveChunk({0, 0, 0}));
		}
	}
}
