#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <optional>

#include "../../platform/window/IWindow.hpp"

namespace render::vulkan {
	constexpr uint32_t vulkanApiVersion = VK_API_VERSION_1_0;

	using DeviceExtensions = std::vector<const char*>;

	struct QueueFamilyIndices {
		std::optional<uint32_t> graphicsFamily;
		std::optional<uint32_t> presentFamily;

		[[nodiscard]] bool isComplete() const;
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
		SwapChainSupportDetails    m_swapChainSupport;
		QueueFamilyIndices         m_queueFamilyIndices;
		VkSampleCountFlagBits      m_msaaSamples = VK_SAMPLE_COUNT_1_BIT;
		std::optional<uint32_t>    m_memoryType;
		bool                       m_samplerAnisotropyEnabled = false;

		void               createInstance();
		void               createSurface();
		void               updateMaxUsableSampleCount();
		[[nodiscard]] bool isSuitable(VkPhysicalDevice device) const;
		static bool        checkExtensionSupport(VkPhysicalDevice device);

	public:
		static const DeviceExtensions deviceExtensions;

		[[nodiscard]] static int deviceTypeScore(VkPhysicalDeviceType type);

		explicit VulkanContext(platform::window::IWindow&);
		~VulkanContext();

		void                   choosePhysicalDevice();
		void                   createLogicalDevice();
		[[nodiscard]] uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const;
		[[nodiscard]] VkFormat findDepthFormat() const;
		[[nodiscard]] VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling,
													VkFormatFeatureFlags        features) const;
		QueueFamilyIndices                         findQueueFamilies(VkPhysicalDevice device) const;
		SwapChainSupportDetails                    querySwapChainSupport(VkPhysicalDevice device) const;
		[[nodiscard]] const QueueFamilyIndices&    getQueueFamilyIndices() const;
		[[nodiscard]] const VkInstance&            getInstance() const;
		[[nodiscard]] const VkSurfaceKHR&          getSurface() const;
		[[nodiscard]] const VkDevice&              getLogicalDevice() const;
		[[nodiscard]] const VkPhysicalDevice&      getPhysicalDevice() const;
		[[nodiscard]] const VkQueue&               getGraphicsQueue() const;
		[[nodiscard]] const VkQueue&               getPresentQueue() const;
		[[nodiscard]] platform::window::IWindow&   getWindow() const;
		[[nodiscard]] const VkSampleCountFlagBits& getMsaaSamples() const;
		[[nodiscard]] bool                         isSamplerAnisotropyEnabled() const;
	};
}
