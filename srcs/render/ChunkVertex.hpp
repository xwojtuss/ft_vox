#pragma once

#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include "game/planet/ChunkSize.hpp"

/**
 * The vertex of a chunk mesh, packed into chunkVertexBytes.
 *
 * Budget at the current settings (16 x 16 x 16 blocks, 2 steps per block, 64 steps per texture):
 *
 *   word 0: position x 6 bits, y 6, z 6 (0 to 32 steps), uv u 7 bits, v 7 (0 to 64 steps)  = 32 of 32 bits
 *   word 1: texture layer 12 bits, face direction 3                                         = 15 of 32 bits
 */
namespace render::chunkvertex {
	constexpr std::size_t   vertexBytes           = 8;
	constexpr int           positionStepsPerBlock = 2;
	constexpr int           uvStepsPerTexture     = 64;
	constexpr std::uint32_t layerBits             = 12;
	constexpr std::uint32_t faceCount             = 6;

	constexpr std::uint32_t bitsPerWord = 32;
	constexpr std::size_t   wordCount   = vertexBytes / sizeof(std::uint32_t);

	enum class FieldId : std::uint8_t { PositionX, PositionY, PositionZ, UvU, UvV, Layer, Direction, Count };

	constexpr std::size_t fieldCount = static_cast<std::size_t>(FieldId::Count);

	struct Field {
		std::uint32_t word{};
		std::uint32_t shift{};
		std::uint32_t bits{};
	};

	[[nodiscard]] constexpr std::uint32_t bitsFor(const std::uint32_t maxValue) {
		return static_cast<std::uint32_t>(std::bit_width(maxValue));
	}

	constexpr std::uint32_t maxPositionX = game::planet::chunkXSize * positionStepsPerBlock;
	constexpr std::uint32_t maxPositionY = game::planet::chunkYSize * positionStepsPerBlock;
	constexpr std::uint32_t maxPositionZ = game::planet::chunkZSize * positionStepsPerBlock;
	constexpr std::uint32_t maxUv        = uvStepsPerTexture;
	constexpr std::uint32_t maxLayer     = (1U << layerBits) - 1;
	constexpr std::uint32_t maxDirection = faceCount - 1;

	constexpr std::array<std::uint32_t, fieldCount> fieldBits = {
		bitsFor(maxPositionX), bitsFor(maxPositionY), bitsFor(maxPositionZ), bitsFor(maxUv), bitsFor(maxUv), layerBits,
		bitsFor(maxDirection)};

	struct Layout {
		std::array<Field, fieldCount> fields{};
		std::size_t                   wordsUsed{};
	};

	[[nodiscard]] constexpr Layout makeLayout() {
		Layout        layout;
		std::uint32_t word = 0;
		std::uint32_t used = 0;

		for (std::size_t i = 0; i < fieldCount; ++i) {
			if (used + fieldBits[i] > bitsPerWord) {
				++word;
				used = 0;
			}
			layout.fields[i] = {.word = word, .shift = used, .bits = fieldBits[i]};
			used += fieldBits[i];
		}
		layout.wordsUsed = word + 1;
		return layout;
	}

	constexpr Layout layout = makeLayout();

	static_assert(vertexBytes % sizeof(std::uint32_t) == 0, "the vertex is made of 32-bit words");
	static_assert(layout.wordsUsed <= wordCount,
				  "the chunk vertex fields do not fit into chunkvertex::vertexBytes: save bits first, and raise "
				  "vertexBytes only as a deliberate decision");
	static_assert(layout.wordsUsed == wordCount,
				  "the chunk vertex fields fit into fewer bytes than chunkvertex::vertexBytes: lower it, so no "
				  "memory is wasted");

	[[nodiscard]] constexpr const Field& field(const FieldId id) {
		return layout.fields[static_cast<std::size_t>(id)];
	}
}

namespace render {
	struct ChunkVertex {
		std::array<std::uint32_t, chunkvertex::wordCount> words{};

		[[nodiscard]] bool operator==(const ChunkVertex&) const = default;
	};

	static_assert(sizeof(ChunkVertex) == chunkvertex::vertexBytes);

	/** Throws error::AssetError when a value is not on the grid of the layout (a block model off its steps). */
	[[nodiscard]] ChunkVertex packChunkVertex(const glm::vec3& position, const glm::vec2& uv, std::uint32_t layer,
											  std::uint32_t direction);

	[[nodiscard]] glm::vec3     chunkVertexPosition(const ChunkVertex& vertex);
	[[nodiscard]] glm::vec2     chunkVertexUv(const ChunkVertex& vertex);
	[[nodiscard]] std::uint32_t chunkVertexLayer(const ChunkVertex& vertex);
	[[nodiscard]] std::uint32_t chunkVertexDirection(const ChunkVertex& vertex);
}
