#include "render/vulkan/VulkanSwapchain.hpp"
#include <GLFW/glfw3.h>
#include <algorithm>
#include <array>
#include <limits>
#include <vulkan/vk_enum_string_helper.h>
#include "app/ApplicationInfo.hpp"
#include "render/vulkan/VulkanError.hpp"
#include "render/vulkan/VulkanContext.hpp"
#include "log/Log.hpp"

using namespace render::vulkan;

VulkanSwapchain::VulkanSwapchain(const VulkanContext& context) {
	createSwapChain(context);
	createImageViews(context);
	createRenderPass(context);
	createColorResources(context);
	createDepthResources(context);
	createFramebuffers(context);
	logging::get(error::Domain::Render)
		.info("Presenting frames with {} (vsync {})", string_VkPresentModeKHR(m_presentMode),
			  app::VSyncEnabled ? "on" : "off");
}

VulkanSwapchain::~VulkanSwapchain() = default;

void VulkanSwapchain::createSwapChain(const VulkanContext& context) {
	const SwapChainSupportDetails swapChainSupport = context.querySwapChainSupport(context.getPhysicalDevice());
	const VkSurfaceFormatKHR      surfaceFormat    = chooseSwapSurfaceFormat(swapChainSupport.formats);
	const VkPresentModeKHR        presentMode = chooseSwapPresentMode(swapChainSupport.presentModes, app::VSyncEnabled);
	const VkExtent2D              extent      = chooseSwapExtent(context.getWindow(), swapChainSupport.capabilities);

	uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
	if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
		imageCount = swapChainSupport.capabilities.maxImageCount;
	}
	VkSwapchainCreateInfoKHR createInfo{};
	createInfo.sType                        = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface                      = context.getSurface();
	createInfo.minImageCount                = imageCount;
	createInfo.imageFormat                  = surfaceFormat.format;
	createInfo.imageColorSpace              = surfaceFormat.colorSpace;
	createInfo.imageExtent                  = extent;
	createInfo.imageArrayLayers             = 1;
	createInfo.imageUsage                   = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	const QueueFamilies& families           = context.getQueueFamilies();
	const std::array     queueFamilyIndices = {families.graphics, families.present};

	if (families.graphics != families.present) {
		createInfo.imageSharingMode      = VK_SHARING_MODE_CONCURRENT;
		createInfo.queueFamilyIndexCount = 2;
		createInfo.pQueueFamilyIndices   = queueFamilyIndices.data();
	} else {
		createInfo.imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE;
		createInfo.queueFamilyIndexCount = 0;
		createInfo.pQueueFamilyIndices   = nullptr;
	}
	createInfo.preTransform   = swapChainSupport.capabilities.currentTransform;
	createInfo.compositeAlpha = chooseCompositeAlpha(swapChainSupport.capabilities.supportedCompositeAlpha);
	createInfo.presentMode    = presentMode;
	createInfo.clipped        = VK_TRUE;
	createInfo.oldSwapchain   = VK_NULL_HANDLE;
	if (const VkResult result = vkCreateSwapchainKHR(context.getLogicalDevice(), &createInfo, nullptr, &m_swapChain);
		result != VK_SUCCESS) {
		throw VulkanError("failed to create swap chain", result);
	}
	vkGetSwapchainImagesKHR(context.getLogicalDevice(), m_swapChain, &imageCount, nullptr);
	m_swapChainImages.resize(imageCount);
	vkGetSwapchainImagesKHR(context.getLogicalDevice(), m_swapChain, &imageCount, m_swapChainImages.data());
	m_swapChainImageFormat = surfaceFormat.format;
	m_swapChainExtent      = extent;
	m_presentMode          = presentMode;
}

VkSurfaceFormatKHR VulkanSwapchain::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
	constexpr VkSurfaceFormatKHR preferred = {.format     = VK_FORMAT_B8G8R8A8_SRGB,
											  .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};

	if (availableFormats.size() == 1 && availableFormats.front().format == VK_FORMAT_UNDEFINED)
		return preferred;

	for (const VkFormat format: {VK_FORMAT_B8G8R8A8_SRGB, VK_FORMAT_R8G8B8A8_SRGB}) {
		for (const VkSurfaceFormatKHR& available: availableFormats) {
			if (available.format == format && available.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
				return available;
		}
	}
	return availableFormats.front();
}

VkCompositeAlphaFlagBitsKHR VulkanSwapchain::chooseCompositeAlpha(
	const VkCompositeAlphaFlagsKHR supportedCompositeAlpha) {
	for (const VkCompositeAlphaFlagBitsKHR mode:
		 {VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR, VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR,
		  VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR, VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR}) {
		if ((supportedCompositeAlpha & mode) != 0)
			return mode;
	}
	return VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
}

VkPresentModeKHR VulkanSwapchain::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes,
														const bool                           vsync) {
	if (!vsync &&
		std::ranges::find(availablePresentModes, VK_PRESENT_MODE_IMMEDIATE_KHR) != availablePresentModes.end())
		return VK_PRESENT_MODE_IMMEDIATE_KHR;
	return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D VulkanSwapchain::chooseSwapExtent(const platform::window::IWindow& window,
											 const VkSurfaceCapabilitiesKHR&  capabilities) {
	if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
		return capabilities.currentExtent;
	}
	int width  = 0;
	int height = 0;
	glfwGetFramebufferSize(static_cast<GLFWwindow*>(window.getHandle()), &width, &height);

	VkExtent2D actualExtent = {.width = static_cast<uint32_t>(width), .height = static_cast<uint32_t>(height)};

	actualExtent.width =
		std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
	actualExtent.height =
		std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

	return actualExtent;
}

void VulkanSwapchain::createImageViews(const VulkanContext& context) {
	m_swapChainImageViews.resize(m_swapChainImages.size());

	for (uint32_t i = 0; i < m_swapChainImages.size(); i++) {
		m_swapChainImageViews[i] = createImageView(context.getLogicalDevice(), m_swapChainImages[i],
												   m_swapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);
	}
}

VkImageView VulkanSwapchain::createImageView(VkDevice device, VkImage image, VkFormat format,
											 VkImageAspectFlags aspectFlags, uint32_t mipLevels) {
	VkImageViewCreateInfo viewInfo{};
	viewInfo.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image                           = image;
	viewInfo.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format                          = format;
	viewInfo.subresourceRange.aspectMask     = aspectFlags;
	viewInfo.subresourceRange.baseMipLevel   = 0;
	viewInfo.subresourceRange.levelCount     = mipLevels;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount     = 1;

	VkImageView imageView = nullptr;
	if (const VkResult result = vkCreateImageView(device, &viewInfo, nullptr, &imageView); result != VK_SUCCESS) {
		throw VulkanError("failed to create image view", result);
	}

	return imageView;
}

void VulkanSwapchain::createRenderPass(const VulkanContext& context) {
	VkAttachmentDescription depthAttachment{};
	depthAttachment.format         = context.findDepthFormat();
	depthAttachment.samples        = context.getMsaaSamples();
	depthAttachment.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.storeOp        = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
	depthAttachment.finalLayout    = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depthAttachmentRef{};
	depthAttachmentRef.attachment = 1;
	depthAttachmentRef.layout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentDescription colorAttachmentResolve{};
	colorAttachmentResolve.format         = m_swapChainImageFormat;
	colorAttachmentResolve.samples        = VK_SAMPLE_COUNT_1_BIT;
	colorAttachmentResolve.loadOp         = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachmentResolve.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachmentResolve.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachmentResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachmentResolve.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachmentResolve.finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference colorAttachmentResolveRef{};
	colorAttachmentResolveRef.attachment = 2;
	colorAttachmentResolveRef.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentDescription colorAttachment{};
	colorAttachment.format         = m_swapChainImageFormat;
	colorAttachment.samples        = context.getMsaaSamples();
	colorAttachment.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp        = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout    = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference colorAttachmentRef{};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass{};
	subpass.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount    = 1;
	subpass.pColorAttachments       = &colorAttachmentRef;
	subpass.pDepthStencilAttachment = &depthAttachmentRef;
	subpass.pResolveAttachments     = &colorAttachmentResolveRef;

	std::array<VkAttachmentDescription, 3> attachments = {colorAttachment, depthAttachment, colorAttachmentResolve};
	VkRenderPassCreateInfo                 renderPassInfo{};
	renderPassInfo.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
	renderPassInfo.pAttachments    = attachments.data();
	renderPassInfo.subpassCount    = 1;
	renderPassInfo.pSubpasses      = &subpass;

	VkSubpassDependency dependency{};
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
	dependency.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	dependency.srcSubpass    = VK_SUBPASS_EXTERNAL;
	dependency.dstStageMask =
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	dependency.dstSubpass    = 0;

	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies   = &dependency;

	if (const VkResult result = vkCreateRenderPass(context.getLogicalDevice(), &renderPassInfo, nullptr, &m_renderPass);
		result != VK_SUCCESS) {
		throw VulkanError("failed to create render pass", result);
	}
}

void VulkanSwapchain::createImage(const VulkanContext& context, VkExtent2D extent, uint32_t mipLevels,
								  VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling,
								  VkImageUsageFlags usage, VmaAllocationCreateFlags allocationFlags,
								  SwapChainImage& swapChainImage) {
	VkImageCreateInfo imageInfo{};
	imageInfo.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType     = VK_IMAGE_TYPE_2D;
	imageInfo.extent.width  = extent.width;
	imageInfo.extent.height = extent.height;
	imageInfo.extent.depth  = 1;
	imageInfo.mipLevels     = mipLevels;
	imageInfo.arrayLayers   = 1;
	imageInfo.format        = format;
	imageInfo.tiling        = tiling;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.usage         = usage;
	imageInfo.samples       = numSamples;
	imageInfo.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;

	VmaAllocationCreateInfo allocationInfo{};
	allocationInfo.usage = VMA_MEMORY_USAGE_AUTO;
	allocationInfo.flags = allocationFlags;

	if (const VkResult result = vmaCreateImage(context.getAllocator(), &imageInfo, &allocationInfo,
											   &swapChainImage.image, &swapChainImage.allocation, nullptr);
		result != VK_SUCCESS) {
		throw VulkanError("failed to create image", result);
	}
}

void VulkanSwapchain::createDepthResources(const VulkanContext& context) {
	const VkFormat depthFormat = context.findDepthFormat();

	createImage(context, m_swapChainExtent, 1, context.getMsaaSamples(), depthFormat, VK_IMAGE_TILING_OPTIMAL,
				VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT, m_depthImage);
	m_depthImage.imageView =
		createImageView(context.getLogicalDevice(), m_depthImage.image, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1);
}

void VulkanSwapchain::createColorResources(const VulkanContext& context) {
	const VkFormat colorFormat = m_swapChainImageFormat;

	createImage(context, m_swapChainExtent, 1, context.getMsaaSamples(), colorFormat, VK_IMAGE_TILING_OPTIMAL,
				VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
				VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT, m_colorImage);
	m_colorImage.imageView =
		createImageView(context.getLogicalDevice(), m_colorImage.image, colorFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);
}

void VulkanSwapchain::createFramebuffers(const VulkanContext& context) {
	m_swapChainFramebuffers.resize(m_swapChainImageViews.size());

	for (size_t i = 0; i < m_swapChainImageViews.size(); i++) {
		std::array<VkImageView, 3> attachments = {m_colorImage.imageView, m_depthImage.imageView,
												  m_swapChainImageViews[i]};

		VkFramebufferCreateInfo framebufferInfo{};
		framebufferInfo.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass      = m_renderPass;
		framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		framebufferInfo.pAttachments    = attachments.data();
		framebufferInfo.width           = m_swapChainExtent.width;
		framebufferInfo.height          = m_swapChainExtent.height;
		framebufferInfo.layers          = 1;

		if (const VkResult result =
				vkCreateFramebuffer(context.getLogicalDevice(), &framebufferInfo, nullptr, &m_swapChainFramebuffers[i]);
			result != VK_SUCCESS) {
			throw VulkanError("failed to create framebuffer", result);
		}
	}
}

void VulkanSwapchain::recreateSwapChain(const VulkanContext& context) {
	context.getWindow().waitUntilNotMinimized();

	vkDeviceWaitIdle(context.getLogicalDevice());

	cleanupSwapChain(context);

	createSwapChain(context);
	createImageViews(context);
	createRenderPass(context);
	createColorResources(context);
	createDepthResources(context);
	createFramebuffers(context);
}

VkExtent2D VulkanSwapchain::getExtent() const {
	return m_swapChainExtent;
}

VkRenderPass VulkanSwapchain::getRenderPass() const {
	return m_renderPass;
}

VkFramebuffer VulkanSwapchain::getFramebuffer(uint32_t index) const {
	return m_swapChainFramebuffers[index];
}

size_t VulkanSwapchain::getImageCount() const {
	return m_swapChainImages.size();
}

void VulkanSwapchain::cleanupSwapChain(const VulkanContext& context) const {
	vkDestroyImageView(context.getLogicalDevice(), m_depthImage.imageView, nullptr);
	vmaDestroyImage(context.getAllocator(), m_depthImage.image, m_depthImage.allocation);

	vkDestroyImageView(context.getLogicalDevice(), m_colorImage.imageView, nullptr);
	vmaDestroyImage(context.getAllocator(), m_colorImage.image, m_colorImage.allocation);

	for (auto* framebuffer: m_swapChainFramebuffers) {
		vkDestroyFramebuffer(context.getLogicalDevice(), framebuffer, nullptr);
	}

	for (auto* imageView: m_swapChainImageViews) {
		vkDestroyImageView(context.getLogicalDevice(), imageView, nullptr);
	}

	vkDestroySwapchainKHR(context.getLogicalDevice(), m_swapChain, nullptr);
	vkDestroyRenderPass(context.getLogicalDevice(), m_renderPass, nullptr);
}

VkSwapchainKHR VulkanSwapchain::getSwapChain() const {
	return m_swapChain;
}

void VulkanSwapchain::cleanup(const VulkanContext& context) const {
	cleanupSwapChain(context);
}
