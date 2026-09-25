#include "VulkanContext.hpp"
#include <algorithm>
#include <functional>
#include <set>
#include <string>
#include <string_view>
#include <GLFW/glfw3.h> // For glfwCreateWindowSurface
#include "VulkanValidationLayers.hpp"
#include "../../app/ApplicationInfo.hpp"
#include "VulkanError.hpp"

using namespace render::vulkan;

namespace {
	constexpr const char* portabilitySubsetExtension = "VK_KHR_portability_subset";
#ifdef VK_KHR_portability_enumeration
	constexpr const char*           portabilityEnumerationExtension = VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME;
	constexpr VkInstanceCreateFlags portabilityEnumerationFlag      = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#else
	constexpr const char*           portabilityEnumerationExtension = "VK_KHR_portability_enumeration";
	constexpr VkInstanceCreateFlags portabilityEnumerationFlag      = 0x00000001;
#endif

	std::vector<VkExtensionProperties> availableInstanceExtensions() {
		uint32_t count = 0;
		vkEnumerateInstanceExtensionProperties(nullptr, &count, nullptr);
		std::vector<VkExtensionProperties> extensions(count);
		vkEnumerateInstanceExtensionProperties(nullptr, &count, extensions.data());
		return extensions;
	}

	std::vector<VkExtensionProperties> availableDeviceExtensions(VkPhysicalDevice device) {
		uint32_t count = 0;
		vkEnumerateDeviceExtensionProperties(device, nullptr, &count, nullptr);
		std::vector<VkExtensionProperties> extensions(count);
		vkEnumerateDeviceExtensionProperties(device, nullptr, &count, extensions.data());
		return extensions;
	}

	bool hasExtension(const std::vector<VkExtensionProperties>& extensions, const std::string_view name) {
		return std::any_of(extensions.begin(), extensions.end(), [name](const VkExtensionProperties& extension) {
			return name == extension.extensionName;
		});
	}

	void addExtensionOnce(std::vector<const char*>& extensions, const char* name) {
		if (const auto sameName = [name](const char* extension) { return std::string_view(extension) == name; }; std::none_of(extensions.begin(), extensions.end(), sameName))
			extensions.push_back(name);
	}
}

bool QueueFamilyIndices::isComplete() const {
	return graphicsFamily.has_value() && presentFamily.has_value();
}

void VulkanContext::createInstance() {
	const std::vector<VkExtensionProperties> availableExtensions = availableInstanceExtensions();

	uint32_t           windowExtensionCount = 0;
	const char* const* windowExtensions     = m_window.getExtensions(&windowExtensionCount);

	std::vector extensions(windowExtensions, windowExtensions + windowExtensionCount);
	VkInstanceCreateFlags    flags = 0;

	if (hasExtension(availableExtensions, portabilityEnumerationExtension)) {
		addExtensionOnce(extensions, portabilityEnumerationExtension);
		flags |= portabilityEnumerationFlag;
		if (hasExtension(availableExtensions, VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME))
			addExtensionOnce(extensions, VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
	}

	VkApplicationInfo appInfo{};
	appInfo.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName   = app::appName;
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName        = "No Engine";
	appInfo.engineVersion      = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion         = VK_API_VERSION_1_0;

	constexpr bool validationEnabled = VulkanValidationLayers::isEnabled && VulkanValidationLayers::checkSupport();

	VkInstanceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.flags = flags;
	createInfo.pApplicationInfo = &appInfo;
	createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
	createInfo.ppEnabledExtensionNames = extensions.data();
	createInfo.enabledLayerCount = validationEnabled ? static_cast<uint32_t>(VulkanValidationLayers::layers.size()) : 0;
	createInfo.ppEnabledLayerNames = validationEnabled ? VulkanValidationLayers::layers.data() : nullptr;

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

bool VulkanContext::isSuitable(VkPhysicalDevice device) const {
	if (!checkExtensionSupport(device) || !findQueueFamilies(device).isComplete())
		return false;

	const SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
	return !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
}

int VulkanContext::deviceTypeScore(const VkPhysicalDeviceType type) {
	switch (type) {
		case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
			return 4;
		case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
			return 3;
		case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
			return 2;
		case VK_PHYSICAL_DEVICE_TYPE_CPU:
			return 1;
		default:
			return 0;
	}
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

	int bestScore = -1;
	for (VkPhysicalDevice device: devices) {
		if (!isSuitable(device))
			continue;

		VkPhysicalDeviceProperties properties;
		vkGetPhysicalDeviceProperties(device, &properties);
		if (const int score = deviceTypeScore(properties.deviceType); score > bestScore) {
			bestScore        = score;
			m_physicalDevice = device;
		}
	}

	if (m_physicalDevice == VK_NULL_HANDLE)
		throw VulkanError("failed to find a GPU that can draw to this window");

	m_queueFamilyIndices = findQueueFamilies(m_physicalDevice);
	m_swapChainSupport   = querySwapChainSupport(m_physicalDevice);
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
	VkPhysicalDeviceFeatures supportedFeatures;
	vkGetPhysicalDeviceFeatures(m_physicalDevice, &supportedFeatures);
	m_samplerAnisotropyEnabled = supportedFeatures.samplerAnisotropy == VK_TRUE;

	VkPhysicalDeviceFeatures deviceFeatures{};
	deviceFeatures.samplerAnisotropy = supportedFeatures.samplerAnisotropy;

	DeviceExtensions enabledExtensions = deviceExtensions;
	if (hasExtension(availableDeviceExtensions(m_physicalDevice), portabilitySubsetExtension))
		enabledExtensions.push_back(portabilitySubsetExtension);

	VkDeviceCreateInfo createInfo{};
	createInfo.sType                = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.pQueueCreateInfos    = queueCreateInfos.data();
	createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());

	createInfo.pEnabledFeatures        = &deviceFeatures;
	createInfo.enabledExtensionCount   = static_cast<uint32_t>(enabledExtensions.size());
	createInfo.ppEnabledExtensionNames = enabledExtensions.data();
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
		{VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT, VK_FORMAT_D16_UNORM},
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

	for (uint32_t i = 0; i < queueFamilyCount; ++i) {
		VkBool32 presentSupport = VK_FALSE;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, m_surface, &presentSupport);

		const bool graphics = (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0;
		if (graphics && presentSupport == VK_TRUE) {
			indices.graphicsFamily = i;
			indices.presentFamily  = i;
			return indices;
		}
		if (graphics && !indices.graphicsFamily)
			indices.graphicsFamily = i;
		if (presentSupport == VK_TRUE && !indices.presentFamily)
			indices.presentFamily = i;
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

bool VulkanContext::isSamplerAnisotropyEnabled() const {
	return m_samplerAnisotropyEnabled;
}
