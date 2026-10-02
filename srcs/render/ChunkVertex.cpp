#include "render/ChunkVertex.hpp"

#include <cmath>
#include <format>
#include <string>

#include "error/Assert.hpp"
#include "error/Exception.hpp"

using namespace render;
using namespace render::chunkvertex;

namespace {
	constexpr float gridTolerance = 1e-4f;

	std::uint32_t quantize(const float value, const int steps, const std::uint32_t maxRaw, const char* what) {
		const float scaled  = value * static_cast<float>(steps);
		const float rounded = std::round(scaled);

		if (std::abs(scaled - rounded) > gridTolerance)
			throw error::AssetError("block model", std::format("{} {} is not a multiple of 1/{}", what, value, steps));
		if (rounded < 0.0f || rounded > static_cast<float>(maxRaw))
			throw error::AssetError("block model", std::format("{} {} is outside of the chunk", what, value));
		return static_cast<std::uint32_t>(rounded);
	}

	std::uint32_t mask(const std::uint32_t bits) {
		return (1U << bits) - 1U;
	}

	void write(ChunkVertex& vertex, const FieldId id, const std::uint32_t value) {
		const Field& placement = field(id);
		DEBUG_ASSERT(value <= mask(placement.bits), "value does not fit into its field", value, placement.bits);
		vertex.words[placement.word] |= value << placement.shift;
	}

	std::uint32_t read(const ChunkVertex& vertex, const FieldId id) {
		const Field& placement = field(id);
		return (vertex.words[placement.word] >> placement.shift) & mask(placement.bits);
	}
}

ChunkVertex render::packChunkVertex(const glm::vec3& position, const glm::vec2& uv, const std::uint32_t layer,
									const std::uint32_t direction) {
	DEBUG_ASSERT(layer <= maxLayer, "texture layer does not fit into the chunk vertex", layer);
	DEBUG_ASSERT(direction <= maxDirection, "face direction does not fit into the chunk vertex", direction);

	ChunkVertex vertex;
	write(vertex, FieldId::PositionX, quantize(position.x, positionStepsPerBlock, maxPositionX, "position x"));
	write(vertex, FieldId::PositionY, quantize(position.y, positionStepsPerBlock, maxPositionY, "position y"));
	write(vertex, FieldId::PositionZ, quantize(position.z, positionStepsPerBlock, maxPositionZ, "position z"));
	write(vertex, FieldId::UvU, quantize(uv.x, uvStepsPerTexture, maxUv, "texture coordinate u"));
	write(vertex, FieldId::UvV, quantize(uv.y, uvStepsPerTexture, maxUv, "texture coordinate v"));
	write(vertex, FieldId::Layer, layer);
	write(vertex, FieldId::Direction, direction);
	return vertex;
}

glm::vec3 render::chunkVertexPosition(const ChunkVertex& vertex) {
	const auto steps = static_cast<float>(positionStepsPerBlock);
	return {static_cast<float>(read(vertex, FieldId::PositionX)) / steps,
			static_cast<float>(read(vertex, FieldId::PositionY)) / steps,
			static_cast<float>(read(vertex, FieldId::PositionZ)) / steps};
}

glm::vec2 render::chunkVertexUv(const ChunkVertex& vertex) {
	const auto steps = static_cast<float>(uvStepsPerTexture);
	return {static_cast<float>(read(vertex, FieldId::UvU)) / steps,
			static_cast<float>(read(vertex, FieldId::UvV)) / steps};
}

std::uint32_t render::chunkVertexLayer(const ChunkVertex& vertex) {
	return read(vertex, FieldId::Layer);
}

std::uint32_t render::chunkVertexDirection(const ChunkVertex& vertex) {
	return read(vertex, FieldId::Direction);
}
