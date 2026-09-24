#pragma once

#include <imgui.h>

#include "../IGui.hpp"

namespace render::vulkan {
	class VulkanContext;
	class VulkanSwapchain;
}

namespace platform::window {
	class IWindow;
}

namespace render::gui::vulkan {
	class ImGuiGui : public IGui {
	private:
		render::vulkan::VulkanContext&   m_context;
		render::vulkan::VulkanSwapchain& m_swapchain;
		platform::window::IWindow&       m_window;
		ImGuiContext*                    m_imguiContext{nullptr};
		VkDescriptorPool                 m_descriptorPool{VK_NULL_HANDLE};
		int                              m_windowDepth{0};
		bool                             m_isInitialized{false};

		void createDescriptorPool();
		void init();

	public:
		ImGuiGui(render::vulkan::VulkanContext& context, render::vulkan::VulkanSwapchain& swapchain,
				platform::window::IWindow&      window);
		~ImGuiGui() override;

		void               beginFrame() override;
		void               endFrame() override;
		bool               beginWindow(const std::string& name, bool* open) override;
		void               endWindow() override;
		bool               beginSection(const std::string& name, bool* open) override;
		void               endSection() override;
		void               text(const std::string& value) override;
		bool               button(const std::string& label) override;
		void               separator() override;
		void               render(VkCommandBuffer commandBuffer) override;
		[[nodiscard]] bool wantsMouseCapture() const override;
		[[nodiscard]] bool wantsKeyboardCapture() const override;
	};
}
