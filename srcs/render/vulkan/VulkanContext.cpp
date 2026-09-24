#include "VulkanContext.hpp"

using namespace render::vulkan;

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

void	VulkanContext::createInstance() {
	#ifndef NDEBUG
	if (VulkanValidationLayers::isEnabled && !VulkanValidationLayers().checkSupport()) {
		throw std::runtime_error("validation layers requested, but not available!");
	}
	#endif

	VkApplicationInfo		appInfo{};
	VkInstanceCreateInfo	createInfo{};

	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = app::appName;
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "No Engine";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_0;

	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;

	uint32_t		extensionCount = 0;
	const char**	extensions;

	extensions = m_window.getExtensions(&extensionCount);

	createInfo.enabledExtensionCount = extensionCount;
	createInfo.ppEnabledExtensionNames = extensions;
	#ifndef NDEBUG
	if (VulkanValidationLayers::isEnabled) {
		createInfo.enabledLayerCount = static_cast<uint32_t>(VulkanValidationLayers::layers.size());
		createInfo.ppEnabledLayerNames = VulkanValidationLayers::layers.data();
	} else {
		#endif
		createInfo.enabledLayerCount = 0;
	#ifndef NDEBUG
	}
	#endif

	VkResult	result = vkCreateInstance(&createInfo, NULL, &m_instance);
	if (result != VK_SUCCESS)
		throw std::runtime_error("failed to create instance!");
}

void	VulkanContext::createSurface() {
	if (glfwCreateWindowSurface(m_instance, reinterpret_cast<GLFWwindow*>(m_window.getHandle()), nullptr, &m_surface) != VK_SUCCESS) {
		throw std::runtime_error("failed to create the window surface!");
	}
}

void	VulkanContext::updateMaxUsableSampleCount() {
	VkPhysicalDeviceProperties physicalDeviceProperties;
	vkGetPhysicalDeviceProperties(m_physicalDevice, &physicalDeviceProperties);

	VkSampleCountFlags counts = physicalDeviceProperties.limits.framebufferColorSampleCounts & physicalDeviceProperties.limits.framebufferDepthSampleCounts;
	if (counts & VK_SAMPLE_COUNT_64_BIT) { m_msaaSamples = VK_SAMPLE_COUNT_64_BIT; }
	else if (counts & VK_SAMPLE_COUNT_32_BIT) { m_msaaSamples = VK_SAMPLE_COUNT_32_BIT; }
	else if (counts & VK_SAMPLE_COUNT_16_BIT) { m_msaaSamples = VK_SAMPLE_COUNT_16_BIT; }
	else if (counts & VK_SAMPLE_COUNT_8_BIT) { m_msaaSamples = VK_SAMPLE_COUNT_8_BIT; }
	else if (counts & VK_SAMPLE_COUNT_4_BIT) { m_msaaSamples = VK_SAMPLE_COUNT_4_BIT; }
	else if (counts & VK_SAMPLE_COUNT_2_BIT) { m_msaaSamples = VK_SAMPLE_COUNT_2_BIT; }
	else { m_msaaSamples = VK_SAMPLE_COUNT_1_BIT; }
}

bool VulkanContext::isSuitable(VkPhysicalDevice device) {
	VkPhysicalDeviceProperties	deviceProperties;
	VkPhysicalDeviceFeatures	deviceFeatures;
	
	m_queueFamilyIndices = findQueueFamilies(device);
	vkGetPhysicalDeviceProperties(device, &deviceProperties);
	vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

	bool	extensionsSupported = checkExtensionSupport(device);
	if (!extensionsSupported) return false;

	VkPhysicalDeviceFeatures supportedFeatures;
	vkGetPhysicalDeviceFeatures(device, &supportedFeatures);
	m_swapChainSupport = querySwapChainSupport(device);
	bool swapChainAdequate = !m_swapChainSupport.formats.empty() && !m_swapChainSupport.presentModes.empty();

	// TODO: add support for more devices, this is a hotfix
	return (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU
		|| deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)
		// && deviceFeatures.geometryShader
		&& m_queueFamilyIndices.isComplete() && swapChainAdequate
		&& supportedFeatures.samplerAnisotropy;
}

bool	VulkanContext::checkExtensionSupport(VkPhysicalDevice device) const {
	uint32_t	extensionCount;

	vkEnumerateDeviceExtensionProperties(device, NULL, &extensionCount, NULL);
	std::vector<VkExtensionProperties>	availableExtensions(extensionCount);
	vkEnumerateDeviceExtensionProperties(device, NULL, &extensionCount, availableExtensions.data());

	std::set<std::string>	requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

	for (std::vector<VkExtensionProperties>::const_iterator it = availableExtensions.begin();
		it != availableExtensions.end();
		it++) {
		requiredExtensions.erase((*it).extensionName);
	}
	return requiredExtensions.empty();
}

SwapChainSupportDetails	VulkanContext::querySwapChainSupport(VkPhysicalDevice device) {
	SwapChainSupportDetails	details;

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, m_surface, &details.capabilities);
	uint32_t	formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &formatCount, NULL);

	if (formatCount != 0) {
		details.formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &formatCount, details.formats.data());
	}
	uint32_t	presentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface, &presentModeCount, nullptr);

	if (presentModeCount != 0) {
		details.presentModes.resize(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface, &presentModeCount, details.presentModes.data());
	}

	return details;
}

QueueFamilyIndices	VulkanContext::findQueueFamilies(VkPhysicalDevice dev) {
	QueueFamilyIndices	indices;
	uint32_t			queueFamilyCount = 0;

	vkGetPhysicalDeviceQueueFamilyProperties(dev, &queueFamilyCount, NULL);
	std::vector<VkQueueFamilyProperties>	queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(dev, &queueFamilyCount, queueFamilies.data());

	int	i = 0;
	for (std::vector<VkQueueFamilyProperties>::const_iterator it = queueFamilies.begin();
		it != queueFamilies.end();
		it++) {
		if ((*it).queueFlags & VK_QUEUE_GRAPHICS_BIT) {
			VkBool32	presentSupport = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(dev, i, m_surface, &presentSupport);
			if (presentSupport) indices.presentFamily = i;
			indices.graphicsFamily = i;
		}
		if (indices.isComplete()) break;
		i++;
	}

	return indices;
}

void VulkanContext::choosePhysicalDevice() {
	uint32_t			deviceCount = 0;
	
	m_physicalDevice = VK_NULL_HANDLE;
	vkEnumeratePhysicalDevices(m_instance, &deviceCount, NULL);
	if (deviceCount == 0) {
		throw std::runtime_error("failed to find GPUs with Vulkan support!");
	}
	std::vector<VkPhysicalDevice>	devices(deviceCount);
	vkEnumeratePhysicalDevices(m_instance, &deviceCount, devices.data());

	for (const auto& dev : devices) {
		if (isSuitable(dev)) {
			m_physicalDevice = dev;
			break;
		}
	}

	if (m_physicalDevice == VK_NULL_HANDLE)
		throw std::runtime_error("failed to find a suitable GPU!");
}

void	VulkanContext::createLogicalDevice() {
	QueueFamilyIndices indices = findQueueFamilies(m_physicalDevice);

	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	std::set<uint32_t> uniqueQueueFamilies = {indices.graphicsFamily.value(), indices.presentFamily.value()};

	float queuePriority = 1.0f;
	for (uint32_t queueFamily : uniqueQueueFamilies) {
		VkDeviceQueueCreateInfo queueCreateInfo{};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = queueFamily;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = &queuePriority;
		queueCreateInfos.push_back(queueCreateInfo);
	}
	VkPhysicalDeviceFeatures	deviceFeatures{};
	deviceFeatures.samplerAnisotropy = VK_TRUE;

	VkDeviceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.pQueueCreateInfos = queueCreateInfos.data();
	createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());

	createInfo.pEnabledFeatures = &deviceFeatures;
	createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
	createInfo.ppEnabledExtensionNames = deviceExtensions.data();
	createInfo.enabledLayerCount = 0;
	if (vkCreateDevice(m_physicalDevice, &createInfo, NULL, &m_logicalDevice) != VK_SUCCESS) {
		throw std::runtime_error("failed to create logical device!");
	}
	vkGetDeviceQueue(m_logicalDevice, indices.graphicsFamily.value(), 0, &m_graphicsQueue);
	vkGetDeviceQueue(m_logicalDevice, indices.presentFamily.value(), 0, &m_presentQueue);
}

VkFormat	VulkanContext::findDepthFormat() const {
	return findSupportedFormat(
		{VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
		VK_IMAGE_TILING_OPTIMAL,
		VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
	);
}

VkFormat	VulkanContext::findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) const {
	for (VkFormat format : candidates) {
		VkFormatProperties props;
		vkGetPhysicalDeviceFormatProperties(m_physicalDevice, format, &props);

		if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
			return format;
		} else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
			return format;
		}
	}

	throw std::runtime_error("failed to find supported format!");
}

bool	QueueFamilyIndices::isComplete() {
	return graphicsFamily.has_value() && presentFamily.has_value();
}

const VkSurfaceKHR&	VulkanContext::getSurface() const {
	return m_surface;
}

const VkPhysicalDevice&	VulkanContext::getPhysicalDevice() const {
	return m_physicalDevice;
}

const VkDevice&	VulkanContext::getLogicalDevice() const {
	return m_logicalDevice;
}

const VkQueue&	VulkanContext::getGraphicsQueue() const {
	return m_graphicsQueue;
}

const VkQueue&	VulkanContext::getPresentQueue() const {
	return m_presentQueue;
}

platform::window::IWindow&	VulkanContext::getWindow() {
	return m_window;
}

const VkSampleCountFlagBits&	VulkanContext::getMsaaSamples() const {
	return m_msaaSamples;
}

const QueueFamilyIndices&	VulkanContext::getQueueFamilyIndices() const {
	return m_queueFamilyIndices;
}

const VkInstance&	VulkanContext::getInstance() const {
	return m_instance;
}

uint32_t	VulkanContext::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
	VkPhysicalDeviceMemoryProperties memProperties;

	vkGetPhysicalDeviceMemoryProperties(m_physicalDevice, &memProperties);

	for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
		if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
			return i;
		}
	}

	throw std::runtime_error("failed to find suitable memory type!");
}

VulkanContext::~VulkanContext() {
	vkDestroyDevice(m_logicalDevice, nullptr);
	vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
	vkDestroyInstance(m_instance, nullptr);
}