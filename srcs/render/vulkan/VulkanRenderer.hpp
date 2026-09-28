#pragma once

#include <vulkan/vulkan.h>
#include <memory>
#include <unordered_map>
#include <vector>

#include "render/vulkan/VulkanContext.hpp"
#include "render/vulkan/VulkanSwapchain.hpp"
#include "render/vulkan/VulkanResourceManager.hpp"
#include "render/vulkan/VulkanFrameData.hpp"
#include "render/vulkan/pipeline/APipeline.hpp"
#include "render/IRenderer.hpp"
#include "platform/window/IWindow.hpp"
#include "ecs/system/SystemManager.hpp"

namespace render::vulkan {
	class VulkanRenderer : public IRenderer {
	private:
		std::unique_ptr<VulkanContext>                                       m_context;
		std::unique_ptr<VulkanSwapchain>                                     m_swapchain;
		std::unique_ptr<VulkanResourceManager>                               m_resourceManager;
		std::unique_ptr<VulkanFrameData>                                     m_frameData;
		std::optional<uint32_t>                                              m_frameIndex;
		std::array<VkClearValue, 2>                                          m_clearValues{};
		std::unordered_map<assets::PipelineType, std::unique_ptr<APipeline>> m_pipelineHandles;
		std::vector<VkSemaphore>                                             m_renderFinishedSemaphores;

		void createPipelines();
		void createRenderFinishedSemaphores();
		void cleanupRenderFinishedSemaphores();
		void recordCurrentCommandBuffer(ecs::SystemManager& systemManager);
		void cleanupPipelines();
		void beginFrame();
		void endFrame();

	public:
		explicit VulkanRenderer(platform::window::IWindow&);
		~VulkanRenderer() override;

		void cleanup() override;

		assets::MeshHandle           createMesh(const assets::MeshData&) override;
		assets::TextureHandle        createTexture(const assets::TextureData&) override;
		void                         render(ecs::SystemManager& systemManager) override;
		void                         render(render::gui::IGui& gui) override;
		void                         setClearColor(float r, float g, float b, float a) override;
		void                         setClearColor(int hexColor) override;
		void                         drawMesh(const ecs::component::Mesh& mesh, const ecs::component::Texture* texture,
											  const ecs::component::Transform& transform) override;
		void                         updateCamera(const ecs::component::Camera& camera) override;
		[[nodiscard]] VulkanContext& getContext() const;
		[[nodiscard]] VulkanSwapchain&           getSwapchain() const;
		[[nodiscard]] platform::window::IWindow& getWindow() const;
	};
}
