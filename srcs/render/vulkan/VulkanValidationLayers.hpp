#pragma once

#include <vector>
#include <vulkan/vulkan.h>

namespace render::vulkan {
	using ValidationLayers = std::vector<const char*>;

	class VulkanValidationLayers {
	public:
		static const ValidationLayers layers;
#ifdef NDEBUG
		static constexpr bool isEnabled = false;
#else
		static constexpr bool isEnabled = true;
#endif

		[[nodiscard]] static bool checkSupport();

		[[nodiscard]] static VkDebugUtilsMessengerCreateInfoEXT messengerCreateInfo();
		[[nodiscard]] static VkDebugUtilsMessengerEXT createMessenger(VkInstance instance);
		static void destroyMessenger(VkInstance instance, VkDebugUtilsMessengerEXT messenger);
	};
}
