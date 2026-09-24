#include "VulkanContext.hpp"
#include <functional>
#include <set>
#include <string>
#include <string_view>
#include <stdexcept>
#include <GLFW/glfw3.h> // For glfwCreateWindowSurface
#include "VulkanValidationLayers.hpp"
#include "../../app/ApplicationInfo.hpp"
#include "VulkanError.hpp"

using namespace render::vulkan;

bool QueueFamilyIndices::isComplete() const {
	return graphicsFamily.has_value() && presentFamily.has_value();
}

void VulkanContext::createInstance() {
#ifndef NDEBUG
	if (VulkanValidationLayers::isEnabled && !VulkanValidationLayers().checkSupport()) {
		throw VulkanError("validation layers requested, but not available");
	}
#endif

	VkApplicationInfo    appInfo{};
	VkInstanceCreateInfo createInfo{};

	appInfo.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName   = app::appName;
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName        = "No Engine";
	appInfo.engineVersion      = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion         = VK_API_VERSION_1_0;

	createInfo.sType            = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;

	uint32_t           extensionCount = 0;
	const char* const* extensions     = nullptr;

	extensions = m_window.getExtensions(&extensionCount);

	createInfo.enabledExtensionCount   = extensionCount;
	createInfo.ppEnabledExtensionNames = extensions;
#ifndef NDEBUG
	if (VulkanValidationLayers::isEnabled) {
		createInfo.enabledLayerCount   = static_cast<uint32_t>(VulkanValidationLayers::layers.size());
		createInfo.ppEnabledLayerNames = VulkanValidationLayers::layers.data();
	} else {
#endif
	createInfo.enabledLayerCount = 0;
#ifndef NDEBUG
	}
#endif

	if (const VkResult result = vkCreateInstance(&createInfo, nullptr, &m_instance); result != VK_SUCCESS)
		throw VulkanError("failed to create instance", result);
}

void VulkanContext::createSurface() {
	if (const VkResult result = glfwCreateWindowSurface(m_instance, static_cast<GLFWwindow*>(m_window.getHandle()),
														nullptr, &m_surface); result != VK_SUCCESS) {
		throw VulkanError("failed to create the window surface", result);
	}
}

void VulkanContext::updateMaxUsableSampleCount() {
	VkPhysicalDeviceProperties physicalDeviceProperties;
	vkGetPhysicalDeviceProperties(m_physicalDevice, &physicalDeviceProperties);

	const VkSampleCountFlags counts = physicalDeviceProperties.limits.framebufferColorSampleCounts &
									physicalDeviceProperties.limits.framebufferDepthSampleCounts;
	if (counts & VK_SAMPLE_COUNT_64_BIT) { m_msaaSamples = VK_SAMPLE_COUNT_64_BIT; } else if (
		counts & VK_SAMPLE_COUNT_32_BIT) { m_msaaSamples = VK_SAMPLE_COUNT_32_BIT; } else if (
		counts & VK_SAMPLE_COUNT_16_BIT) { m_msaaSamples = VK_SAMPLE_COUNT_16_BIT; } else if (
		counts & VK_SAMPLE_COUNT_8_BIT) { m_msaaSamples = VK_SAMPLE_COUNT_8_BIT; } else if (
		counts & VK_SAMPLE_COUNT_4_BIT) { m_msaaSamples = VK_SAMPLE_COUNT_4_BIT; } else if (
		counts & VK_SAMPLE_COUNT_2_BIT) { m_msaaSamples = VK_SAMPLE_COUNT_2_BIT; } else {
		m_msaaSamples = VK_SAMPLE_COUNT_1_BIT;
	}
}

bool VulkanContext::isSuitable(VkPhysicalDevice device) {
	VkPhysicalDeviceProperties deviceProperties;
	VkPhysicalDeviceFeatures   deviceFeatures;

	m_queueFamilyIndices = findQueueFamilies(device);
	vkGetPhysicalDeviceProperties(device, &deviceProperties);
	vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

	if (const bool extensionsSupported = checkExtensionSupport(device); !extensionsSupported)
		return false;

	VkPhysicalDeviceFeatures supportedFeatures;
	vkGetPhysicalDeviceFeatures(device, &supportedFeatures);
	m_swapChainSupport           = querySwapChainSupport(device);
	const bool swapChainAdequate = !m_swapChainSupport.formats.empty() && !m_swapChainSupport.presentModes.empty();

	// TODO: add support for more devices, this is a hotfix
	return (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU
			|| deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)
			// && deviceFeatures.geometryShader
			&& m_queueFamilyIndices.isComplete() && swapChainAdequate
			&& supportedFeatures.samplerAnisotropy;
}

bool VulkanContext::checkExtensionSupport(VkPhysicalDevice device) {
	uint32_t extensionCount = 0;

	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
	std::vector<VkExtensionProperties> availableExtensions(extensionCount);
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

	std::set<std::string, std::less<>> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

	for (const auto& [extensionName, _]: availableExtensions) {
		if (const auto required = requiredExtensions.find(std::string_view(extensionName));
			required != requiredExtensions.end())
			requiredExtensions.erase(required);
	}
	return requiredExtensions.empty();
}

const DeviceExtensions VulkanContext::deviceExtensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

VulkanContext::VulkanContext(platform::window::IWindow& window) : m_window(window) {
	createInstance();
	createSurface();
	choosePhysicalDevice();
	updateMaxUsableSampleCount();
	createLogicalDevice();
}

VulkanContext::~VulkanContext() {
	vkDestroyDevice(m_logicalDevice, nullptr);
	vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
	vkDestroyInstance(m_instance, nullptr);
}

void VulkanContext::choosePhysicalDevice() {
	uint32_t deviceCount = 0;

	m_physicalDevice = VK_NULL_HANDLE;
	vkEnumeratePhysicalDevices(m_instance, &deviceCount, nullptr);
	if (deviceCount == 0) {
		throw VulkanError("failed to find GPUs with Vulkan support");
	}
	std::vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(m_instance, &deviceCount, devices.data());

	for (const auto& dev: devices) {
		if (isSuitable(dev)) {
			m_physicalDevice = dev;
			break;
		}
	}

	if (m_physicalDevice == VK_NULL_HANDLE)
		throw VulkanError("failed to find a suitable GPU");
}

void VulkanContext::createLogicalDevice() {
	QueueFamilyIndices indices = findQueueFamilies(m_physicalDevice);

	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	const std::set uniqueQueueFamilies = {indices.graphicsFamily.value(), indices.presentFamily.value()};

	constexpr float queuePriority = 1.0f;
	for (const uint32_t queueFamily: uniqueQueueFamilies) {
		VkDeviceQueueCreateInfo queueCreateInfo{};
		queueCreateInfo.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = queueFamily;
		queueCreateInfo.queueCount       = 1;
		queueCreateInfo.pQueuePriorities = &queuePriority;
		queueCreateInfos.push_back(queueCreateInfo);
	}
	VkPhysicalDeviceFeatures deviceFeatures{};
	deviceFeatures.samplerAnisotropy = VK_TRUE;

	VkDeviceCreateInfo createInfo{};
	createInfo.sType                = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.pQueueCreateInfos    = queueCreateInfos.data();
	createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());

	createInfo.pEnabledFeatures        = &deviceFeatures;
	createInfo.enabledExtensionCount   = static_cast<uint32_t>(deviceExtensions.size());
	createInfo.ppEnabledExtensionNames = deviceExtensions.data();
	createInfo.enabledLayerCount       = 0;
	if (const VkResult result = vkCreateDevice(m_physicalDevice, &createInfo, nullptr, &m_logicalDevice);
		result != VK_SUCCESS) {
		throw VulkanError("failed to create logical device", result);
	}
	vkGetDeviceQueue(m_logicalDevice, indices.graphicsFamily.value(), 0, &m_graphicsQueue);
	vkGetDeviceQueue(m_logicalDevice, indices.presentFamily.value(), 0, &m_presentQueue);
}

uint32_t VulkanContext::findMemoryType(const uint32_t typeFilter, const VkMemoryPropertyFlags properties) const {
	VkPhysicalDeviceMemoryProperties memProperties;

	vkGetPhysicalDeviceMemoryProperties(m_physicalDevice, &memProperties);

	for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
		if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
			return i;
		}
	}

	throw VulkanError("failed to find suitable memory type");
}

VkFormat VulkanContext::findDepthFormat() const {
	return findSupportedFormat(
		{VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
		VK_IMAGE_TILING_OPTIMAL,
		VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
	);
}

VkFormat VulkanContext::findSupportedFormat(const std::vector<VkFormat>& candidates, const VkImageTiling tiling,
											const VkFormatFeatureFlags   features) const {
	for (const VkFormat format: candidates) {
		VkFormatProperties props;
		vkGetPhysicalDeviceFormatProperties(m_physicalDevice, format, &props);

		if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
			return format;
		}
		if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
			return format;
		}
	}

	throw VulkanError("failed to find supported format");
}

QueueFamilyIndices VulkanContext::findQueueFamilies(VkPhysicalDevice device) const {
	QueueFamilyIndices indices;
	uint32_t           queueFamilyCount = 0;

	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

	int i = 0;
	for (const auto& queueFamily: queueFamilies) {
		if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
			VkBool32 presentSupport = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(device, i, m_surface, &presentSupport);
			if (presentSupport)
				indices.presentFamily = i;
			indices.graphicsFamily = i;
		}
		if (indices.isComplete())
			break;
		i++;
	}

	return indices;
}

SwapChainSupportDetails VulkanContext::querySwapChainSupport(VkPhysicalDevice device) const {
	SwapChainSupportDetails details;

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, m_surface, &details.capabilities);
	uint32_t formatCount = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &formatCount, nullptr);

	if (formatCount != 0) {
		details.formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &formatCount, details.formats.data());
	}
	uint32_t presentModeCount = 0;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface, &presentModeCount, nullptr);

	if (presentModeCount != 0) {
		details.presentModes.resize(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface, &presentModeCount, details.presentModes.data());
	}

	return details;
}

const QueueFamilyIndices& VulkanContext::getQueueFamilyIndices() const {
	return m_queueFamilyIndices;
}

const VkInstance& VulkanContext::getInstance() const {
	return m_instance;
}

const VkSurfaceKHR& VulkanContext::getSurface() const {
	return m_surface;
}

const VkDevice& VulkanContext::getLogicalDevice() const {
	return m_logicalDevice;
}

const VkPhysicalDevice& VulkanContext::getPhysicalDevice() const {
	return m_physicalDevice;
}

const VkQueue& VulkanContext::getGraphicsQueue() const {
	return m_graphicsQueue;
}

const VkQueue& VulkanContext::getPresentQueue() const {
	return m_presentQueue;
}

platform::window::IWindow& VulkanContext::getWindow() const {
	return m_window;
}

const VkSampleCountFlagBits& VulkanContext::getMsaaSamples() const {
	return m_msaaSamples;
}
