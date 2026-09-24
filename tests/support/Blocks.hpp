#pragma once

#include <array>

#include "assets/Resources.hpp"
#include "game/block/BlockData.hpp"

namespace test {

constexpr game::BlockId	air = 0;
constexpr game::BlockId	dirt = 1;
constexpr game::BlockId	blockWithoutModel = 2;

constexpr glm::vec3		cubeColor = {1.0f, 0.5f, 0.25f};

/**
 * A unit cube from (0, 0, 0) to (1, 1, 1): 6 faces, 12 triangles,
 * wound counter-clockwise when seen from outside so normals point outwards.
 */
inline assets::MeshData	makeCubeMesh() {
	using Quad = std::array<glm::vec3, 4>;
	const std::array<Quad, 6> faces = {{
		{{{1, 0, 0}, {1, 1, 0}, {1, 1, 1}, {1, 0, 1}}}, // +x
		{{{0, 0, 0}, {0, 0, 1}, {0, 1, 1}, {0, 1, 0}}}, // -x
		{{{0, 1, 0}, {0, 1, 1}, {1, 1, 1}, {1, 1, 0}}}, // +y
		{{{0, 0, 0}, {1, 0, 0}, {1, 0, 1}, {0, 0, 1}}}, // -y
		{{{0, 0, 1}, {1, 0, 1}, {1, 1, 1}, {0, 1, 1}}}, // +z
		{{{0, 0, 0}, {0, 1, 0}, {1, 1, 0}, {1, 0, 0}}}, // -z
	}};
	const std::array<glm::vec2, 4> texCoords = {{{0, 0}, {0, 1}, {1, 1}, {1, 0}}};
	assets::MeshData mesh;

	for (const Quad& face : faces) {
		const uint32_t first = static_cast<uint32_t>(mesh.vertices.size());
		for (size_t corner = 0; corner < 4; ++corner)
			mesh.vertices.push_back({face[corner], cubeColor, texCoords[corner]});
		mesh.indices.insert(mesh.indices.end(), {first, first + 1, first + 2, first, first + 2, first + 3});
	}
	return mesh;
}

/**
 * Block registry where dirt uses the unit cube and `pixels` as its texture
 */
inline game::block::BlockDatas	makeBlockDatas(void* pixels = nullptr) {
	assets::TextureData texture{};
	texture.width = 1;
	texture.height = 1;
	texture.mipLevels = 1;
	texture.pixels = pixels;
	return game::block::BlockDatas(makeCubeMesh(), texture);
}
}
