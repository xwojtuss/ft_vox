#pragma once

#include <array>
#include <volk.h>

namespace render::vulkan {
	class VulkanValidationLayers {
	public:
		static constexpr std::array layers = {"VK_LAYER_KHRONOS_validation"};
#ifdef NDEBUG
		static constexpr bool isEnabled = false;
#else
		static constexpr bool isEnabled = true;
#endif

		[[nodiscard]] static bool checkSupport();

		[[nodiscard]] static VkDebugUtilsMessengerCreateInfoEXT messengerCreateInfo();
		[[nodiscard]] static VkDebugUtilsMessengerEXT           createMessenger(VkInstance instance);
		static void destroyMessenger(VkInstance instance, VkDebugUtilsMessengerEXT messenger);
	};
}
