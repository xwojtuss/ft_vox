#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include "assets/TinyObjLoader.hpp"
#include "game/world/ChunkMesher.hpp"
#include "support/Blocks.hpp"

using game::Block;
using game::world::Chunk;
using game::world::ChunkMesher;
using game::world::chunkXSize;
using game::world::chunkYSize;
using game::world::chunkZSize;

namespace {
	size_t triangleCount(const assets::MeshData& mesh) {
		return mesh.indices.size() / 3;
	}

	void fill(Chunk& chunk, glm::ivec3 from, glm::ivec3 to, game::BlockId id) {
		for (int x = from.x; x <= to.x; ++x)
			for (int y = from.y; y <= to.y; ++y)
				for (int z = from.z; z <= to.z; ++z)
					chunk.setBlock(static_cast<unsigned short>(x),
									static_cast<unsigned short>(y),
									static_cast<unsigned short>(z),
									Block(id));
	}
}

SCENARIO("An empty chunk produces no geometry", "[mesher]") {
	GIVEN("a chunk full of air") {
		game::block::BlockDatas blockDatas = test::makeBlockDatas();
		ChunkMesher             mesher(blockDatas);
		const Chunk             chunk;

		WHEN("it is meshed") {
			const assets::MeshData mesh = mesher.toMeshData(chunk);

			THEN("there is nothing to draw") {
				REQUIRE(mesh.vertices.empty());
				REQUIRE(mesh.indices.empty());
			}
		}
	}
}

SCENARIO("A lone block shows all six faces at its own position", "[mesher]") {
	GIVEN("a chunk with a single dirt block at (8, 8, 8)") {
		game::block::BlockDatas blockDatas = test::makeBlockDatas();
		ChunkMesher             mesher(blockDatas);
		Chunk                   chunk;
		chunk.setBlock(8, 8, 8, Block(test::dirt));

		WHEN("it is meshed") {
			const assets::MeshData mesh = mesher.toMeshData(chunk);

			THEN("the whole cube is drawn: 6 faces, 12 triangles") {
				REQUIRE(triangleCount(mesh) == 12);
				REQUIRE(mesh.vertices.size() == 36);
			}
			AND_THEN("every vertex is moved to the block's position inside the chunk") {
				for (const render::Vertex& vertex: mesh.vertices) {
					REQUIRE(vertex.pos.x >= 8.0f);
					REQUIRE(vertex.pos.x <= 9.0f);
					REQUIRE(vertex.pos.y >= 8.0f);
					REQUIRE(vertex.pos.y <= 9.0f);
					REQUIRE(vertex.pos.z >= 8.0f);
					REQUIRE(vertex.pos.z <= 9.0f);
				}
			}
			AND_THEN("colors and texture coordinates come from the block model") {
				for (const render::Vertex& vertex: mesh.vertices) {
					REQUIRE(vertex.color == test::cubeColor);
					REQUIRE(vertex.texCoord.x >= 0.0f);
					REQUIRE(vertex.texCoord.x <= 1.0f);
					REQUIRE(vertex.texCoord.y >= 0.0f);
					REQUIRE(vertex.texCoord.y <= 1.0f);
				}
			}
			AND_THEN("every index points at a vertex of this mesh") {
				for (uint32_t index: mesh.indices)
					REQUIRE(index < mesh.vertices.size());
			}
		}
	}
}

SCENARIO("Faces between two touching blocks are hidden", "[mesher]") {
	GIVEN("two dirt blocks side by side at (8, 8, 8) and (9, 8, 8)") {
		game::block::BlockDatas blockDatas = test::makeBlockDatas();
		ChunkMesher             mesher(blockDatas);
		Chunk                   chunk;
		chunk.setBlock(8, 8, 8, Block(test::dirt));
		chunk.setBlock(9, 8, 8, Block(test::dirt));

		WHEN("they are meshed") {
			const assets::MeshData mesh = mesher.toMeshData(chunk);

			THEN("only the 10 outer faces are drawn") {
				REQUIRE(triangleCount(mesh) == 10 * 2);
			}
			AND_THEN("no triangle lies on the shared face at x = 9") {
				for (size_t i = 0; i < mesh.indices.size(); i += 3) {
					const bool onSharedFace = mesh.vertices[mesh.indices[i]].pos.x == 9.0f
											&& mesh.vertices[mesh.indices[i + 1]].pos.x == 9.0f
											&& mesh.vertices[mesh.indices[i + 2]].pos.x == 9.0f;
					REQUIRE_FALSE(onSharedFace);
				}
			}
		}
	}
}

SCENARIO("Only the surface of a solid shape is drawn", "[mesher]") {
	game::block::BlockDatas blockDatas = test::makeBlockDatas();
	ChunkMesher             mesher(blockDatas);

	GIVEN("a solid 3 x 3 x 3 cube of dirt in the middle of a chunk") {
		Chunk chunk;
		fill(chunk, {4, 4, 4}, {6, 6, 6}, test::dirt);

		THEN("only its outside is drawn: 6 sides of 3 x 3 faces") {
			REQUIRE(triangleCount(mesher.toMeshData(chunk)) == 6 * 9 * 2);
		}
	}

	GIVEN("a chunk completely filled with dirt") {
		Chunk chunk;
		fill(chunk, {0, 0, 0}, {chunkXSize - 1, chunkYSize - 1, chunkZSize - 1}, test::dirt);

		THEN("the chunk's borders count as air, so its whole outer shell is drawn") {
			REQUIRE(triangleCount(mesher.toMeshData(chunk)) == 6 * chunkXSize * chunkZSize * 2);
		}
	}
}

SCENARIO("Blocks without a usable model are invisible", "[mesher]") {
	game::block::BlockDatas blockDatas = test::makeBlockDatas();
	ChunkMesher             mesher(blockDatas);
	Chunk                   chunk;

	GIVEN("a block type that has no model") {
		chunk.setBlock(8, 8, 8, Block(test::blockWithoutModel));

		THEN("nothing is drawn for it") {
			REQUIRE(mesher.toMeshData(chunk).vertices.empty());
		}
	}

	GIVEN("a block model with fewer than 3 indices") {
		blockDatas.getBlockData(test::dirt).meshData.indices = {0, 1};
		chunk.setBlock(8, 8, 8, Block(test::dirt));

		THEN("nothing is drawn for it") {
			REQUIRE(mesher.toMeshData(chunk).vertices.empty());
		}
	}

	GIVEN("a block model with indices but no vertices") {
		blockDatas.getBlockData(test::dirt).meshData.vertices.clear();
		chunk.setBlock(8, 8, 8, Block(test::dirt));

		THEN("nothing is drawn for it") {
			REQUIRE(mesher.toMeshData(chunk).vertices.empty());
		}
	}
}

SCENARIO("Broken triangles in a block model are skipped", "[mesher]") {
	game::block::BlockDatas blockDatas = test::makeBlockDatas();
	ChunkMesher             mesher(blockDatas);
	Chunk                   chunk;
	assets::MeshData&       dirtModel = blockDatas.getBlockData(test::dirt).meshData;
	chunk.setBlock(8, 8, 8, Block(test::dirt));

	GIVEN("a cube model where one triangle points at a vertex that does not exist") {
		dirtModel.indices[1] = 999;

		THEN("that triangle is skipped and the other 11 are drawn") {
			REQUIRE(triangleCount(mesher.toMeshData(chunk)) == 11);
		}
	}

	GIVEN("a cube model with an extra flat triangle (all three corners the same)") {
		dirtModel.indices.insert(dirtModel.indices.end(), {0, 0, 0});

		THEN("the flat triangle is skipped and the cube is drawn as usual") {
			REQUIRE(triangleCount(mesher.toMeshData(chunk)) == 12);
		}
	}
}

SCENARIO("The game's block model fills exactly its own cell", "[mesher][assets]") {
	GIVEN("the cube model the game loads for its blocks") {
		assets::TinyObjLoader   loader;
		const assets::MeshData  cube = loader.toMeshData("models/cube.obj");
		game::block::BlockDatas blockDatas(cube, assets::TextureData{});
		ChunkMesher             mesher(blockDatas);

		THEN("every corner lies between 0 and 1 on every axis") {
			for (const render::Vertex& vertex: cube.vertices) {
				for (int axis = 0; axis < 3; ++axis) {
					REQUIRE(vertex.pos[axis] >= 0.0f);
					REQUIRE(vertex.pos[axis] <= 1.0f);
				}
			}
		}

		WHEN("two blocks touch along the x, the y or the z axis") {
			const glm::ivec3 direction = GENERATE(glm::ivec3(1, 0, 0), glm::ivec3(0, 1, 0), glm::ivec3(0, 0, 1));
			Chunk            chunk;
			chunk.setBlock(8, 8, 8, Block(test::dirt));
			chunk.setBlock(8 + direction.x, 8 + direction.y, 8 + direction.z, Block(test::dirt));

			THEN("the faces between them are hidden and only the 10 outer faces are drawn") {
				REQUIRE(triangleCount(mesher.toMeshData(chunk)) == 10 * 2);
			}
		}
	}
}
