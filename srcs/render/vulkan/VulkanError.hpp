#pragma once

#include <optional>
#include <string>
#include <vulkan/vulkan.h>

#include "../../error/Exception.hpp"

namespace render::vulkan {
	class VulkanError : public error::Exception {
	private:
		std::optional<VkResult> m_result;

	public:
		explicit VulkanError(const std::string& message);
		VulkanError(const std::string& message, VkResult result);

		[[nodiscard]] std::optional<VkResult> result() const noexcept;
	};
}
