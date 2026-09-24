#pragma once

#include <vector>

#include "render/IRenderer.hpp"

namespace test {

/**
 * Stands in for the GPU: records what the game asks the renderer to create
 * so tests can check it, and ignores every draw call.
 */
class FakeRenderer : public render::IRenderer {
public:
	std::vector<size_t>					createdMeshTriangleCounts;
	std::vector<assets::MeshHandle>		createdMeshes;
	std::vector<void*>					createdTexturePixels;
	std::vector<assets::TextureHandle>	createdTextures;
	assets::MeshHandle					textMeshHandle;

	assets::MeshHandle	createMesh(const assets::MeshData& meshData) override {
		createdMeshTriangleCounts.push_back(meshData.indices.size() / 3);
		createdMeshes.emplace_back();
		return createdMeshes.back();
	}

	assets::TextureHandle	createTexture(const assets::TextureData& textureData) override {
		createdTexturePixels.push_back(textureData.pixels);
		createdTextures.emplace_back();
		return createdTextures.back();
	}

	const assets::MeshHandle&	getTextMeshHandle() const override { return textMeshHandle; }
	void	render(ecs::SystemManager&) override {}
	void	render(render::gui::IGui&) override {}
	void	setClearColor(float, float, float, float) override {}
	void	setClearColor(int) override {}
	void	cleanup() override {}
	void	drawMesh(const ecs::component::Mesh&, const ecs::component::Texture*, const ecs::component::Transform&) override {}
	void	drawText(const ecs::component::Text&, const ecs::component::Texture*, const ecs::component::Transform2D&, size_t, const ecs::component::Color*) override {}
	void	updateCamera(const ecs::component::Camera&) override {}
};
}
