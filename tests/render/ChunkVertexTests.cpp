#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include "error/Exception.hpp"
#include "render/ChunkVertex.hpp"

using game::planet::chunkXSize;
using game::planet::chunkYSize;
using game::planet::chunkZSize;
namespace chunkvertex = render::chunkvertex;

namespace {
	constexpr std::uint32_t usedBits = 47;

	std::uint32_t usedBitsOfLayout() {
		std::uint32_t total = 0;
		for (const std::uint32_t bits: chunkvertex::fieldBits)
			total += bits;
		return total;
	}
}

SCENARIO("A chunk vertex is 8 bytes and its layout is pinned", "[render][chunk-vertex]") {
	GIVEN("the current chunk vertex settings") {
		THEN("the vertex takes 8 bytes") {
			REQUIRE(sizeof(render::ChunkVertex) == 8);
		}
		AND_THEN("the fields use 47 bits, so any change to the layout shows up in this test") {
			REQUIRE(usedBitsOfLayout() == usedBits);
		}
		AND_THEN("no field crosses a 32-bit word") {
			for (const chunkvertex::Field& field: chunkvertex::layout.fields)
				REQUIRE(field.shift + field.bits <= chunkvertex::bitsPerWord);
		}
		AND_THEN("no two fields share a bit") {
			std::array<std::uint32_t, chunkvertex::wordCount> taken{};
			for (const chunkvertex::Field& field: chunkvertex::layout.fields) {
				const std::uint32_t mask = ((1U << field.bits) - 1U) << field.shift;
				REQUIRE((taken[field.word] & mask) == 0);
				taken[field.word] |= mask;
			}
		}
	}
}

SCENARIO("A chunk vertex keeps what was packed into it", "[render][chunk-vertex]") {
	GIVEN("a vertex in the middle of a block face") {
		const render::ChunkVertex vertex = render::packChunkVertex({3.5f, 7.0f, 12.5f}, {0.25f, 0.75f}, 5, 4);

		THEN("the position comes back exactly, half blocks included") {
			REQUIRE(render::chunkVertexPosition(vertex) == glm::vec3(3.5f, 7.0f, 12.5f));
		}
		AND_THEN("the texture coordinates, layer and direction come back") {
			REQUIRE(render::chunkVertexUv(vertex) == glm::vec2(0.25f, 0.75f));
			REQUIRE(render::chunkVertexLayer(vertex) == 5);
			REQUIRE(render::chunkVertexDirection(vertex) == 4);
		}
	}

	GIVEN("a vertex at the far corner of the chunk with the largest layer and direction") {
		const render::ChunkVertex vertex =
			render::packChunkVertex(glm::vec3(chunkXSize, chunkYSize, chunkZSize), {1.0f, 1.0f}, chunkvertex::maxLayer,
									chunkvertex::maxDirection);

		THEN("every field still holds its largest value") {
			REQUIRE(render::chunkVertexPosition(vertex) == glm::vec3(chunkXSize, chunkYSize, chunkZSize));
			REQUIRE(render::chunkVertexUv(vertex) == glm::vec2(1.0f, 1.0f));
			REQUIRE(render::chunkVertexLayer(vertex) == chunkvertex::maxLayer);
			REQUIRE(render::chunkVertexDirection(vertex) == chunkvertex::maxDirection);
		}
	}

	GIVEN("a vertex at the origin") {
		const render::ChunkVertex vertex = render::packChunkVertex({0.0f, 0.0f, 0.0f}, {0.0f, 0.0f}, 0, 0);

		THEN("all bits are zero") {
			for (const std::uint32_t word: vertex.words)
				REQUIRE(word == 0);
		}
	}

	GIVEN("two vertices that differ in a single field") {
		const render::ChunkVertex a = render::packChunkVertex({1.0f, 1.0f, 1.0f}, {0.5f, 0.5f}, 1, 1);
		const render::ChunkVertex b = render::packChunkVertex({1.0f, 1.0f, 1.0f}, {0.5f, 0.5f}, 2, 1);

		THEN("they are different and only the layer differs") {
			REQUIRE_FALSE(a == b);
			REQUIRE(render::chunkVertexPosition(a) == render::chunkVertexPosition(b));
			REQUIRE(render::chunkVertexUv(a) == render::chunkVertexUv(b));
			REQUIRE(render::chunkVertexDirection(a) == render::chunkVertexDirection(b));
		}
	}
}

SCENARIO("Values the layout cannot hold are refused", "[render][chunk-vertex]") {
	GIVEN("a position between two steps of a block") {
		const float offGrid = 1.0f / (static_cast<float>(chunkvertex::positionStepsPerBlock) * 3.0f);

		THEN("packing fails, so a block model off the grid is found when it is meshed") {
			REQUIRE_THROWS_AS(render::packChunkVertex({offGrid, 0.0f, 0.0f}, {0.0f, 0.0f}, 0, 0), error::AssetError);
		}
	}

	GIVEN("a texture coordinate between two steps of the texture") {
		THEN("packing fails") {
			REQUIRE_THROWS_AS(render::packChunkVertex({0.0f, 0.0f, 0.0f}, {0.001f, 0.0f}, 0, 0), error::AssetError);
		}
	}

	GIVEN("a position outside of the chunk") {
		const glm::vec3 outside = GENERATE(glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, chunkYSize + 1.0f, 0.0f),
										   glm::vec3(0.0f, 0.0f, chunkZSize + 1.0f));

		THEN("packing fails") {
			REQUIRE_THROWS_AS(render::packChunkVertex(outside, {0.0f, 0.0f}, 0, 0), error::AssetError);
		}
	}
}
