#pragma once

#include <volk.h>
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
#include "profiling/Profiler.hpp"
#include "ecs/system/SystemManager.hpp"

namespace render::vulkan {
	struct DrawBatchKey {
		assets::PipelineType pipelineType{};
		const GpuTexture*    texture{};
		uint32_t             arena{};

		[[nodiscard]] bool operator==(const DrawBatchKey&) const = default;
	};

	struct DrawItem {
		uint32_t  indexCount{};
		uint32_t  firstIndex{};
		int32_t   vertexOffset{};
		glm::mat4 model;
	};

	struct DrawBatch {
		DrawBatchKey          key;
		std::vector<DrawItem> items;
		size_t                firstCommand{};
	};

	class VulkanRenderer : public IRenderer {
	private:
		std::unique_ptr<VulkanContext>                                       m_context;
		std::unique_ptr<VulkanSwapchain>                                     m_swapchain;
		std::unique_ptr<VulkanResourceManager>                               m_resourceManager;
		std::unique_ptr<VulkanFrameData>                                     m_frameData;
		profiling::GpuProfiler                                               m_gpuProfiler;
		std::array<VkClearValue, 2>                                          m_clearValues{};
		std::unordered_map<assets::PipelineType, std::unique_ptr<APipeline>> m_pipelineHandles;
		std::vector<VkSemaphore>                                             m_renderFinishedSemaphores;
		std::vector<DrawBatch>                                               m_batches;
		size_t                                                               m_currentBatch{};
		std::vector<ObjectData>                                              m_objects;
		std::vector<VkDrawIndexedIndirectCommand>                            m_commands;

		void                     createPipelines();
		void                     createRenderFinishedSemaphores();
		void                     cleanupRenderFinishedSemaphores();
		void                     recordCommandBuffer(ecs::SystemManager& systemManager, uint32_t imageIndex);
		void                     flushDraws();
		[[nodiscard]] DrawBatch& batchFor(const DrawBatchKey& key);
		void                     drawBatch(const APipeline* pipeline, const DrawBatch& batch) const;
		void                     cleanupPipelines();
		void                     recreateSwapchain();
		[[nodiscard]] std::optional<uint32_t> acquireImage();
		void                                  present(uint32_t imageIndex);

	public:
		explicit VulkanRenderer(platform::window::IWindow&);
		VulkanRenderer(const VulkanRenderer&)            = delete;
		VulkanRenderer& operator=(const VulkanRenderer&) = delete;
		VulkanRenderer(VulkanRenderer&&)                 = delete;
		VulkanRenderer& operator=(VulkanRenderer&&)      = delete;
		~VulkanRenderer() override;

		void cleanup() override;

		assets::MeshHandle           createMesh(const assets::MeshData&) override;
		void                         destroyMesh(assets::MeshHandle) override;
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
