#include "VulkanValidationLayers.hpp"

#include <cstring> // for strcmp

#include "../../log/Log.hpp"

using namespace render::vulkan;

namespace {
	VKAPI_ATTR VkBool32 VKAPI_CALL logValidationMessage(const VkDebugUtilsMessageSeverityFlagBitsEXT severity,
														VkDebugUtilsMessageTypeFlagsEXT,
														const VkDebugUtilsMessengerCallbackDataEXT* data, void*) {
		const logging::Logger logger = logging::get(error::Domain::Render);

		if ((severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) != 0)
			logger.error("Vulkan validation: {}", data->pMessage);
		else
			logger.warn("Vulkan validation: {}", data->pMessage);
		return VK_FALSE;
	}
}

const ValidationLayers VulkanValidationLayers::layers = {
	"VK_LAYER_KHRONOS_validation"
};

bool VulkanValidationLayers::checkSupport() {
	uint32_t layerCount = 0;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

	std::vector<VkLayerProperties> availableLayers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

	for (const char* layerName: layers) {
		bool layerFound = false;

		for (const auto& layerProperties: availableLayers) {
			if (strcmp(layerName, layerProperties.layerName) == 0) {
				layerFound = true;
				break;
			}
		}

		if (!layerFound) {
			return false;
		}
	}

	return true;
}

VkDebugUtilsMessengerCreateInfoEXT VulkanValidationLayers::messengerCreateInfo() {
	VkDebugUtilsMessengerCreateInfoEXT createInfo{};
	createInfo.sType           = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
								| VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
							VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
							| VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	createInfo.pfnUserCallback = logValidationMessage;
	return createInfo;
}

VkDebugUtilsMessengerEXT VulkanValidationLayers::createMessenger(VkInstance instance) {
	const auto create = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
		vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"));
	VkDebugUtilsMessengerEXT messenger = VK_NULL_HANDLE;

	if (const VkDebugUtilsMessengerCreateInfoEXT createInfo = messengerCreateInfo();
		create == nullptr || create(instance, &createInfo, nullptr, &messenger) != VK_SUCCESS) {
		logging::get(error::Domain::Render).warn("Vulkan validation messages cannot be logged, they go to stdout");
		return VK_NULL_HANDLE;
	}
	return messenger;
}

void VulkanValidationLayers::destroyMessenger(VkInstance instance, VkDebugUtilsMessengerEXT messenger) {
	if (messenger == VK_NULL_HANDLE)
		return;
	if (const auto destroy = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
		vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT")))
		destroy(instance, messenger, nullptr);
}
