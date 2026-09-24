#pragma once

#include <vector>
#include <vulkan/vulkan.h>

#include "../../platform/window/IWindow.hpp"

namespace render::vulkan {
	class VulkanContext;

	struct SwapChainImage {
		VkImage        image;
		VkImageView    imageView;
		VkDeviceMemory imageMemory;
	};

	class VulkanSwapchain {
	private:
		VkSwapchainKHR             m_swapChain{};
		std::vector<VkImage>       m_swapChainImages;
		std::vector<VkImageView>   m_swapChainImageViews;
		VkFormat                   m_swapChainImageFormat{};
		VkExtent2D                 m_swapChainExtent{};
		VkRenderPass               m_renderPass{};
		std::vector<VkFramebuffer> m_swapChainFramebuffers;
		SwapChainImage             m_colorImage{};
		SwapChainImage             m_depthImage{};

		void createSwapChain(const VulkanContext&);
		void createImageViews(const VulkanContext&);
		void createDepthResources(const VulkanContext& context);
		void createColorResources(const VulkanContext& context);
		void createRenderPass(const VulkanContext&);
		void createFramebuffers(const VulkanContext&);
		void cleanupSwapChain(const VulkanContext& context) const;

		static VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
		static VkPresentModeKHR   chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
		static VkExtent2D         chooseSwapExtent(const platform::window::IWindow& window,
											const VkSurfaceCapabilitiesKHR&         capabilities);

	public:
		explicit VulkanSwapchain(const VulkanContext&);
		~VulkanSwapchain();

		[[nodiscard]] VkExtent2D     getExtent() const;
		[[nodiscard]] VkRenderPass   getRenderPass() const;
		[[nodiscard]] VkSwapchainKHR getSwapChain() const;
		[[nodiscard]] VkFramebuffer  getFramebuffer(uint32_t index) const;
		[[nodiscard]] size_t         getImageCount() const;
		void                         recreateSwapChain(const VulkanContext& context);
		void                         cleanup(const VulkanContext& context) const;

		static void createImage(const VulkanContext&  context, VkExtent2D          extent, uint32_t      mipLevels,
								VkSampleCountFlagBits numSamples, VkFormat         format, VkImageTiling tiling,
								VkImageUsageFlags     usage, VkMemoryPropertyFlags properties,
								SwapChainImage&       swapChainImage);
		static VkImageView createImageView(VkDevice            device, VkImage       image, VkFormat format,
											VkImageAspectFlags aspectFlags, uint32_t mipLevels);
	};
}
