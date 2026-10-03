#include <catch2/catch_test_macros.hpp>

#include <array>
#include <glm/vec3.hpp>
#include <utility>

#include "game/planet/Chunk.hpp"
#include "support/Assertions.hpp"
#include "support/Blocks.hpp"

using game::Block;
using game::planet::Chunk;
using game::planet::ChunkFace;
using game::planet::chunkXSize;
using game::planet::chunkYSize;
using game::planet::chunkZSize;

namespace {
	game::BlockId uniqueIdFor(int x, int y, int z) {
		return 1 + (x * chunkYSize * chunkZSize) + (y * chunkZSize) + z;
	}
}

SCENARIO("A chunk is a 16 x 16 x 16 cube of blocks", "[chunk]") {
	THEN("its size is 16 blocks on every axis") {
		STATIC_REQUIRE(chunkXSize == 16);
		STATIC_REQUIRE(chunkYSize == 16);
		STATIC_REQUIRE(chunkZSize == 16);
	}
}

SCENARIO("A new chunk is filled with air", "[chunk]") {
	GIVEN("a freshly created chunk") {
		const Chunk chunk;

		THEN("every block in it is air") {
			for (int x = 0; x < chunkXSize; ++x)
				for (int y = 0; y < chunkYSize; ++y)
					for (int z = 0; z < chunkZSize; ++z)
						REQUIRE(chunk.getBlock(x, y, z).id == test::air);
		}
	}
}

SCENARIO("Every position in a chunk stores its own block", "[chunk]") {
	GIVEN("a chunk where every position holds a different block type") {
		Chunk chunk;

		for (int x = 0; x < chunkXSize; ++x)
			for (int y = 0; y < chunkYSize; ++y)
				for (int z = 0; z < chunkZSize; ++z)
					chunk.setBlock(x, y, z, Block(uniqueIdFor(x, y, z)));

		THEN("reading any position returns exactly the block stored there") {
			const Chunk& readOnly = chunk;

			for (int x = 0; x < chunkXSize; ++x)
				for (int y = 0; y < chunkYSize; ++y)
					for (int z = 0; z < chunkZSize; ++z)
						REQUIRE(readOnly.getBlock(x, y, z).id == uniqueIdFor(x, y, z));
		}
	}
}

SCENARIO("Removing a block leaves air behind", "[chunk]") {
	GIVEN("a chunk with dirt at (3, 4, 5) and at its neighbour (3, 4, 6)") {
		Chunk chunk;
		chunk.setBlock(3, 4, 5, Block(test::dirt));
		chunk.setBlock(3, 4, 6, Block(test::dirt));

		REQUIRE(chunk.getBlock(3, 4, 5).id == test::dirt);

		WHEN("the block at (3, 4, 5) is removed") {
			chunk.removeBlock(3, 4, 5);

			THEN("that position holds air") {
				REQUIRE(chunk.getBlock(3, 4, 5).id == test::air);
			}
			AND_THEN("the neighbouring block is untouched") {
				REQUIRE(chunk.getBlock(3, 4, 6).id == test::dirt);
			}
		}
	}
}

SCENARIO("A removed block forgets its entity", "[chunk]") {
	GIVEN("a block linked to an entity") {
		Chunk chunk;
		Block block(test::dirt);
		block.entity = ecs::Entity{7};
		chunk.setBlock(0, 0, 0, block);

		REQUIRE(chunk.getBlock(0, 0, 0).hasEntity());

		WHEN("it is removed") {
			chunk.removeBlock(0, 0, 0);

			THEN("the position is no longer linked to an entity") {
				REQUIRE_FALSE(chunk.getBlock(0, 0, 0).hasEntity());
			}
		}
	}
}

SCENARIO("A chunk of only air stores no block data", "[chunk]") {
	GIVEN("a freshly created chunk") {
		Chunk chunk;

		THEN("it is empty and takes no block data") {
			REQUIRE(chunk.isEmpty());
			REQUIRE_FALSE(chunk.isFull());
			REQUIRE(chunk.dataBytes() == 0);
		}

		WHEN("air is placed into it") {
			chunk.setBlock(1, 2, 3, Block(test::air));

			THEN("it still takes no block data") {
				REQUIRE(chunk.isEmpty());
				REQUIRE(chunk.dataBytes() == 0);
			}
		}

		WHEN("a block is placed into it") {
			chunk.setBlock(1, 2, 3, Block(test::dirt));

			THEN("the chunk is no longer empty and holds block data") {
				REQUIRE_FALSE(chunk.isEmpty());
				REQUIRE(chunk.dataBytes() > 0);
				REQUIRE(chunk.getBlock(1, 2, 3).id == test::dirt);
				REQUIRE(chunk.getBlock(0, 0, 0).id == test::air);
			}

			AND_WHEN("that block is removed again") {
				chunk.removeBlock(1, 2, 3);

				THEN("the block data is given back") {
					REQUIRE(chunk.isEmpty());
					REQUIRE(chunk.dataBytes() == 0);
				}
			}

			AND_WHEN("it is replaced by another block") {
				chunk.setBlock(1, 2, 3, Block(test::blockWithoutModel));

				THEN("the chunk still counts one block") {
					chunk.removeBlock(1, 2, 3);
					REQUIRE(chunk.isEmpty());
				}
			}
		}
	}
}

SCENARIO("A chunk knows when it is completely filled", "[chunk]") {
	GIVEN("a chunk filled with dirt except for one block") {
		Chunk chunk;
		for (int x = 0; x < chunkXSize; ++x)
			for (int y = 0; y < chunkYSize; ++y)
				for (int z = 0; z < chunkZSize; ++z)
					chunk.setBlock(x, y, z, Block(test::dirt));
		chunk.removeBlock(5, 5, 5);

		THEN("it is not full") {
			REQUIRE_FALSE(chunk.isFull());
		}

		WHEN("the missing block is placed") {
			chunk.setBlock(5, 5, 5, Block(test::dirt));

			THEN("it is full") {
				REQUIRE(chunk.isFull());
			}
		}
	}
}

SCENARIO("A chunk knows whether one of its sides is solid", "[chunk]") {
	GIVEN("a chunk whose bottom layer (y = 0) is dirt") {
		Chunk chunk;
		for (int x = 0; x < chunkXSize; ++x)
			for (int z = 0; z < chunkZSize; ++z)
				chunk.setBlock(x, 0, z, Block(test::dirt));

		THEN("its bottom side is solid and its top side is not") {
			REQUIRE(chunk.isFaceSolid(ChunkFace::NegativeY));
			REQUIRE_FALSE(chunk.isFaceSolid(ChunkFace::PositiveY));
		}
		AND_THEN("its other sides are not solid, since they hold only one row of dirt each") {
			REQUIRE_FALSE(chunk.isFaceSolid(ChunkFace::NegativeX));
			REQUIRE_FALSE(chunk.isFaceSolid(ChunkFace::PositiveX));
			REQUIRE_FALSE(chunk.isFaceSolid(ChunkFace::NegativeZ));
			REQUIRE_FALSE(chunk.isFaceSolid(ChunkFace::PositiveZ));
		}

		WHEN("one block of the bottom layer is removed") {
			chunk.removeBlock(7, 0, 9);

			THEN("the bottom side is no longer solid") {
				REQUIRE_FALSE(chunk.isFaceSolid(ChunkFace::NegativeY));
			}
		}
	}

	GIVEN("a chunk with a wall of dirt at its largest x") {
		Chunk chunk;
		for (int y = 0; y < chunkYSize; ++y)
			for (int z = 0; z < chunkZSize; ++z)
				chunk.setBlock(chunkXSize - 1, y, z, Block(test::dirt));

		THEN("its positive x side is solid and its negative x side is not") {
			REQUIRE(chunk.isFaceSolid(ChunkFace::PositiveX));
			REQUIRE_FALSE(chunk.isFaceSolid(ChunkFace::NegativeX));
		}
	}

	GIVEN("an empty chunk") {
		const Chunk chunk;

		THEN("no side is solid") {
			REQUIRE_FALSE(chunk.isFaceSolid(ChunkFace::NegativeX));
			REQUIRE_FALSE(chunk.isFaceSolid(ChunkFace::PositiveY));
			REQUIRE_FALSE(chunk.isFaceSolid(ChunkFace::PositiveZ));
		}
	}

	GIVEN("opposite sides") {
		THEN("each face is the opposite of its opposite") {
			REQUIRE(opposite(ChunkFace::PositiveX) == ChunkFace::NegativeX);
			REQUIRE(opposite(ChunkFace::NegativeY) == ChunkFace::PositiveY);
			REQUIRE(opposite(opposite(ChunkFace::PositiveZ)) == ChunkFace::PositiveZ);
		}
	}
}

SCENARIO("A copy of a chunk is a snapshot that later edits do not change", "[chunk]") {
	GIVEN("a chunk with one dirt block and a copy of it") {
		Chunk chunk;
		chunk.setBlock(1, 2, 3, Block(test::dirt));
		const Chunk snapshot = chunk;

		WHEN("the original is edited") {
			chunk.setBlock(1, 2, 3, Block(test::air));
			chunk.setBlock(4, 5, 6, Block(test::dirt));

			THEN("the copy still holds the blocks it had when it was taken") {
				REQUIRE(snapshot.getBlock(1, 2, 3).id == test::dirt);
				REQUIRE(snapshot.getBlock(4, 5, 6).id == test::air);
			}
			AND_THEN("the original holds the new blocks") {
				REQUIRE(chunk.getBlock(1, 2, 3).id == test::air);
				REQUIRE(chunk.getBlock(4, 5, 6).id == test::dirt);
			}
		}

		WHEN("the copy is edited") {
			Chunk edited = snapshot;
			edited.setBlock(7, 7, 7, Block(test::dirt));

			THEN("the original is untouched") {
				REQUIRE(chunk.getBlock(7, 7, 7).id == test::air);
			}
		}
	}

	GIVEN("a chunk that was copied while empty") {
		Chunk       chunk;
		const Chunk snapshot = chunk;

		WHEN("the original is edited") {
			chunk.setBlock(0, 0, 0, Block(test::dirt));

			THEN("the copy stays empty") {
				REQUIRE(snapshot.isEmpty());
			}
		}
	}
}

SCENARIO("A block knows which faces of its chunk it touches", "[chunk]") {
	using game::planet::chunkFaceCount;
	using game::planet::isBlockOnChunkFace;

	GIVEN("the block in the corner at the origin") {
		const glm::ivec3 block(0, 0, 0);

		THEN("it touches the three negative faces and none of the positive ones") {
			REQUIRE(isBlockOnChunkFace(block, ChunkFace::NegativeX));
			REQUIRE(isBlockOnChunkFace(block, ChunkFace::NegativeY));
			REQUIRE(isBlockOnChunkFace(block, ChunkFace::NegativeZ));
			REQUIRE_FALSE(isBlockOnChunkFace(block, ChunkFace::PositiveX));
			REQUIRE_FALSE(isBlockOnChunkFace(block, ChunkFace::PositiveY));
			REQUIRE_FALSE(isBlockOnChunkFace(block, ChunkFace::PositiveZ));
		}
	}

	GIVEN("the block in the opposite corner") {
		const glm::ivec3 block(chunkXSize - 1, chunkYSize - 1, chunkZSize - 1);

		THEN("it touches the three positive faces and none of the negative ones") {
			REQUIRE(isBlockOnChunkFace(block, ChunkFace::PositiveX));
			REQUIRE(isBlockOnChunkFace(block, ChunkFace::PositiveY));
			REQUIRE(isBlockOnChunkFace(block, ChunkFace::PositiveZ));
			REQUIRE_FALSE(isBlockOnChunkFace(block, ChunkFace::NegativeX));
			REQUIRE_FALSE(isBlockOnChunkFace(block, ChunkFace::NegativeY));
			REQUIRE_FALSE(isBlockOnChunkFace(block, ChunkFace::NegativeZ));
		}
	}

	GIVEN("a block in the middle of the chunk") {
		THEN("it touches no face") {
			for (std::size_t face = 0; face < chunkFaceCount; ++face)
				REQUIRE_FALSE(isBlockOnChunkFace({8, 8, 8}, static_cast<ChunkFace>(face)));
		}
	}
}

SCENARIO("The step to a neighbouring chunk matches the face", "[chunk]") {
	using game::planet::chunkFaceCount;
	using game::planet::chunkFaceOffsets;

	GIVEN("the offsets of all six faces") {
		THEN("each is a single step along one axis") {
			for (const glm::ivec3& offset: chunkFaceOffsets)
				REQUIRE(std::abs(offset.x) + std::abs(offset.y) + std::abs(offset.z) == 1);
		}
		AND_THEN("the offsets of two opposite faces cancel out") {
			for (std::size_t face = 0; face < chunkFaceCount; ++face) {
				const auto other = static_cast<std::size_t>(std::to_underlying(opposite(static_cast<ChunkFace>(face))));
				REQUIRE(chunkFaceOffsets[face] + chunkFaceOffsets[other] == glm::ivec3(0));
			}
		}
		AND_THEN("the positive faces step towards larger coordinates") {
			REQUIRE(chunkFaceOffsets[std::to_underlying(ChunkFace::PositiveX)].x == 1);
			REQUIRE(chunkFaceOffsets[std::to_underlying(ChunkFace::PositiveY)].y == 1);
			REQUIRE(chunkFaceOffsets[std::to_underlying(ChunkFace::NegativeY)].y == -1);
		}
	}
}
