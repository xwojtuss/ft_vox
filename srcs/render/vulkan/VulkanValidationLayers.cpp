#include "VulkanValidationLayers.hpp"

#include "gui/PlayerComponentsPanel.hpp"

using namespace render::vulkan;

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
