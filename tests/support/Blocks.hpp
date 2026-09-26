#pragma once

#include <array>
#include <utility>
#include <vector>

#include "assets/Resources.hpp"
#include "game/block/BlockData.hpp"

namespace test {
	constexpr game::BlockId air               = 0;
	constexpr game::BlockId dirt              = 1;
	constexpr game::BlockId blockWithoutModel = 2;

	constexpr glm::vec3 cubeColor = {1.0f, 0.5f, 0.25f};

	inline assets::MeshData makeCubeMesh() {
		using CounterClockwiseQuad               = std::array<glm::vec3, 4>;
		const CounterClockwiseQuad     right     = {{{1, 0, 0}, {1, 1, 0}, {1, 1, 1}, {1, 0, 1}}};
		const CounterClockwiseQuad     left      = {{{0, 0, 0}, {0, 0, 1}, {0, 1, 1}, {0, 1, 0}}};
		const CounterClockwiseQuad     top       = {{{0, 1, 0}, {0, 1, 1}, {1, 1, 1}, {1, 1, 0}}};
		const CounterClockwiseQuad     bottom    = {{{0, 0, 0}, {1, 0, 0}, {1, 0, 1}, {0, 0, 1}}};
		const CounterClockwiseQuad     back      = {{{0, 0, 1}, {1, 0, 1}, {1, 1, 1}, {0, 1, 1}}};
		const CounterClockwiseQuad     front     = {{{0, 0, 0}, {0, 1, 0}, {1, 1, 0}, {1, 0, 0}}};
		const std::array               faces     = {right, left, top, bottom, back, front};
		const std::array<glm::vec2, 4> texCoords = {{{0, 0}, {0, 1}, {1, 1}, {1, 0}}};
		assets::MeshData               mesh;

		for (const CounterClockwiseQuad& face: faces) {
			const auto first = static_cast<uint32_t>(mesh.vertices.size());
			for (size_t corner = 0; corner < 4; ++corner)
				mesh.vertices.push_back({face[corner], cubeColor, texCoords[corner]});
			mesh.indices.insert(mesh.indices.end(), {first, first + 1, first + 2, first, first + 2, first + 3});
		}
		return mesh;
	}

	inline game::block::BlockDatas makeBlockDatas(std::vector<unsigned char> dirtTexturePixels = {0, 0, 0, 255}) {
		assets::TextureData texture{};
		texture.width     = 1;
		texture.height    = 1;
		texture.mipLevels = 1;
		texture.pixels    = std::move(dirtTexturePixels);
		return game::block::BlockDatas(makeCubeMesh(), texture);
	}
}
