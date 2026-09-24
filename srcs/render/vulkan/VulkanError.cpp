#include "VulkanError.hpp"

#include <vulkan/vk_enum_string_helper.h>

using namespace render::vulkan;

VulkanError::VulkanError(const std::string& message) : Exception(error::Domain::Render, message) {
}

VulkanError::VulkanError(const std::string& message, VkResult result)
	: Exception(error::Domain::Render, message + " (" + string_VkResult(result) + ")"), m_result(result) {
}

std::optional<VkResult> VulkanError::result() const noexcept {
	return m_result;
}
