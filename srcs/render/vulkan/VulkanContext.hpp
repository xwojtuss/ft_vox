#pragma once

#include <vk_mem_alloc.h>
#include <array>
#include <vector>
#include <optional>

#include "platform/window/IWindow.hpp"

namespace render::vulkan {
	constexpr uint32_t vulkanApiVersion = VK_API_VERSION_1_0;

	constexpr VkSampleCountFlagBits maxMsaaSamples = VK_SAMPLE_COUNT_4_BIT;

	using DeviceExtensions = std::vector<const char*>;

	struct QueueFamilyIndices {
		std::optional<uint32_t> graphicsFamily;
		std::optional<uint32_t> presentFamily;

		[[nodiscard]] bool isComplete() const;
	};

	struct QueueFamilies {
		uint32_t graphics;
		uint32_t present;
	};

	struct SwapChainSupportDetails {
		VkSurfaceCapabilitiesKHR        capabilities{};
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR>   presentModes;
	};

	class VulkanContext {
	private:
		platform::window::IWindow& m_window;
		VkInstance                 m_instance{};
		VkPhysicalDevice           m_physicalDevice{};
		VkDevice                   m_logicalDevice{};
		VkQueue                    m_graphicsQueue{};
		VkQueue                    m_presentQueue{};
		VkSurfaceKHR               m_surface{};
		VkDebugUtilsMessengerEXT   m_debugMessenger{};
		SwapChainSupportDetails    m_swapChainSupport;
		QueueFamilies              m_queueFamilies{};
		VkSampleCountFlagBits      m_msaaSamples = VK_SAMPLE_COUNT_1_BIT;
		VmaAllocator               m_allocator{};
		bool                       m_samplerAnisotropyEnabled = false;
		bool                       m_indirectDrawEnabled      = false;
		uint32_t                   m_maxDrawIndirectCount     = 1;

		void               createInstance();
		void               createSurface();
		void               updateMaxUsableSampleCount();
		[[nodiscard]] bool isSuitable(VkPhysicalDevice device) const;
		static bool        checkExtensionSupport(VkPhysicalDevice device);

	public:
		static constexpr std::array deviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

		[[nodiscard]] static int deviceTypeScore(VkPhysicalDeviceType type);

		explicit VulkanContext(platform::window::IWindow&);
		VulkanContext(const VulkanContext&)            = delete;
		VulkanContext& operator=(const VulkanContext&) = delete;
		VulkanContext(VulkanContext&&)                 = delete;
		VulkanContext& operator=(VulkanContext&&)      = delete;
		~VulkanContext();

		void                    choosePhysicalDevice();
		void                    createLogicalDevice();
		[[nodiscard]] VkFormat  findDepthFormat() const;
		[[nodiscard]] VkFormat  findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling,
													VkFormatFeatureFlags features) const;
		QueueFamilyIndices      findQueueFamilies(VkPhysicalDevice device) const;
		SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device) const;
		[[nodiscard]] const QueueFamilies&         getQueueFamilies() const;
		[[nodiscard]] const VkInstance&            getInstance() const;
		[[nodiscard]] const VkSurfaceKHR&          getSurface() const;
		[[nodiscard]] const VkDevice&              getLogicalDevice() const;
		[[nodiscard]] const VkPhysicalDevice&      getPhysicalDevice() const;
		[[nodiscard]] VmaAllocator                 getAllocator() const;
		[[nodiscard]] const VkQueue&               getGraphicsQueue() const;
		[[nodiscard]] const VkQueue&               getPresentQueue() const;
		[[nodiscard]] platform::window::IWindow&   getWindow() const;
		[[nodiscard]] const VkSampleCountFlagBits& getMsaaSamples() const;
		[[nodiscard]] bool                         isSamplerAnisotropyEnabled() const;
		[[nodiscard]] bool                         isIndirectDrawEnabled() const;
		[[nodiscard]] uint32_t                     getMaxDrawIndirectCount() const;
	};
}
