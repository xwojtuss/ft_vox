#pragma once

#include <string>
#include <vector>

#include "render/IRenderer.hpp"

namespace test {

struct DrawnMesh {
	uint64_t						meshId;
	const ecs::component::Texture*	texture;
	glm::vec3						position;
};

struct DrawnText {
	std::string						text;
	size_t							offset;
	glm::vec2						position;
	const ecs::component::Texture*	texture;
	const ecs::component::Color*	color;
};

class FakeRenderer : public render::IRenderer {
public:
	std::vector<size_t>					createdMeshTriangleCounts;
	std::vector<assets::MeshHandle>		createdMeshes;
	std::vector<void*>					createdTexturePixels;
	std::vector<assets::TextureHandle>	createdTextures;
	std::vector<DrawnMesh>				drawnMeshes;
	std::vector<DrawnText>				drawnTexts;
	std::vector<glm::mat4>				cameraViews;
	int									guiRenders = 0;
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

	void	drawMesh(const ecs::component::Mesh& mesh, const ecs::component::Texture* texture, const ecs::component::Transform& transform) override {
		drawnMeshes.push_back({mesh.mesh.id, texture, transform.position});
	}

	void	drawText(const ecs::component::Text& text, const ecs::component::Texture* texture, const ecs::component::Transform2D& transform, size_t offset, const ecs::component::Color* color) override {
		drawnTexts.push_back({text.text, offset, transform.position, texture, color});
	}

	void	updateCamera(const ecs::component::Camera& camera) override {
		cameraViews.push_back(camera.view);
	}

	void	render(render::gui::IGui&) override {
		++guiRenders;
	}

	const assets::MeshHandle&	getTextMeshHandle() const override { return textMeshHandle; }
	void	render(ecs::SystemManager&) override {}
	void	setClearColor(float, float, float, float) override {}
	void	setClearColor(int) override {}
	void	cleanup() override {}
};
}
