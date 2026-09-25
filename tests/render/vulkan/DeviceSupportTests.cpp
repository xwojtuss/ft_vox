#include <catch2/catch_test_macros.hpp>

#include "render/vulkan/VulkanContext.hpp"
#include "render/vulkan/VulkanSwapchain.hpp"

using render::vulkan::VulkanContext;
using render::vulkan::VulkanSwapchain;

SCENARIO("The most capable kind of GPU is preferred, but any kind can be used", "[render][vulkan]") {
	THEN("a discrete GPU beats an integrated one, which beats a virtual one, which beats a software renderer") {
		REQUIRE(VulkanContext::deviceTypeScore(VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) >
				VulkanContext::deviceTypeScore(VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU));
		REQUIRE(VulkanContext::deviceTypeScore(VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU) >
				VulkanContext::deviceTypeScore(VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU));
		REQUIRE(VulkanContext::deviceTypeScore(VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU) >
				VulkanContext::deviceTypeScore(VK_PHYSICAL_DEVICE_TYPE_CPU));
	}
	AND_THEN("even an unknown kind of GPU still counts as usable") {
		REQUIRE(VulkanContext::deviceTypeScore(VK_PHYSICAL_DEVICE_TYPE_OTHER) >= 0);
	}
}

SCENARIO("The window uses an sRGB format whenever the display offers one", "[render][vulkan]") {
	constexpr VkSurfaceFormatKHR bgraSrgb   = {VK_FORMAT_B8G8R8A8_SRGB, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};
	constexpr VkSurfaceFormatKHR rgbaSrgb   = {VK_FORMAT_R8G8B8A8_SRGB, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};
	constexpr VkSurfaceFormatKHR bgraLinear = {VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};

	THEN("BGRA sRGB is chosen when available") {
		const VkSurfaceFormatKHR chosen = VulkanSwapchain::chooseSwapSurfaceFormat({bgraLinear, rgbaSrgb, bgraSrgb});
		REQUIRE(chosen.format == VK_FORMAT_B8G8R8A8_SRGB);
	}
	AND_THEN("RGBA sRGB is used on displays that only offer that order") {
		const VkSurfaceFormatKHR chosen = VulkanSwapchain::chooseSwapSurfaceFormat({bgraLinear, rgbaSrgb});
		REQUIRE(chosen.format == VK_FORMAT_R8G8B8A8_SRGB);
	}
	AND_THEN("the first offered format is used when there is no sRGB format") {
		const VkSurfaceFormatKHR chosen = VulkanSwapchain::chooseSwapSurfaceFormat({bgraLinear});
		REQUIRE(chosen.format == VK_FORMAT_B8G8R8A8_UNORM);
	}
	AND_THEN("sRGB is chosen when an old driver says any format is fine") {
		const VkSurfaceFormatKHR chosen = VulkanSwapchain::chooseSwapSurfaceFormat({
			{VK_FORMAT_UNDEFINED, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR}
		});
		REQUIRE(chosen.format == VK_FORMAT_B8G8R8A8_SRGB);
		REQUIRE(chosen.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR);
	}
}

SCENARIO("The window blends with the desktop only in ways the system supports", "[render][vulkan]") {
	THEN("an opaque window is used whenever it is supported") {
		REQUIRE(VulkanSwapchain::chooseCompositeAlpha(
			VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR | VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR) == VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR);
	}
	AND_THEN("a system that only supports inheriting the mode gets that instead") {
		REQUIRE(VulkanSwapchain::chooseCompositeAlpha(VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR) ==
				VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR);
	}
	AND_THEN("a system that only supports pre-multiplied alpha gets that instead") {
		REQUIRE(VulkanSwapchain::chooseCompositeAlpha(VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR) ==
				VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR);
	}
}

SCENARIO("Presenting frames never depends on an optional mode", "[render][vulkan]") {
	THEN("FIFO, which every GPU supports, is used when the preferred mode is missing") {
		REQUIRE(VulkanSwapchain::chooseSwapPresentMode({VK_PRESENT_MODE_FIFO_KHR}) == VK_PRESENT_MODE_FIFO_KHR);
	}
}
