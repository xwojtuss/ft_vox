#pragma once

#include <vector>

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

		static bool checkSupport();
	};
}
