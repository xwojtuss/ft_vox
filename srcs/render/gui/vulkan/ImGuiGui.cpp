#include "render/gui/vulkan/ImGuiGui.hpp"
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>
#include "render/vulkan/VulkanError.hpp"
#include "render/vulkan/VulkanContext.hpp"
#include "render/vulkan/VulkanSwapchain.hpp"
#include "platform/window/IWindow.hpp"

#include <GLFW/glfw3.h>

using namespace render::gui::vulkan;

void ImGuiGui::createDescriptorPool() {
	constexpr std::size_t poolSize = 1000;

	const std::array pool_sizes = {
		VkDescriptorPoolSize{.type = VK_DESCRIPTOR_TYPE_SAMPLER, .descriptorCount = poolSize},
		VkDescriptorPoolSize{.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .descriptorCount = poolSize},
		VkDescriptorPoolSize{.type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, .descriptorCount = poolSize},
		VkDescriptorPoolSize{.type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, .descriptorCount = poolSize},
		VkDescriptorPoolSize{.type = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, .descriptorCount = poolSize},
		VkDescriptorPoolSize{.type = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, .descriptorCount = poolSize},
		VkDescriptorPoolSize{.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .descriptorCount = poolSize},
		VkDescriptorPoolSize{.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, .descriptorCount = poolSize},
		VkDescriptorPoolSize{.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, .descriptorCount = poolSize},
		VkDescriptorPoolSize{.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, .descriptorCount = poolSize},
		VkDescriptorPoolSize{.type = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, .descriptorCount = poolSize},
	};

	VkDescriptorPoolCreateInfo pool_info = {};
	pool_info.sType                      = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	pool_info.flags                      = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	pool_info.maxSets                    = poolSize;
	pool_info.poolSizeCount              = std::size(pool_sizes);
	pool_info.pPoolSizes                 = pool_sizes.data();

	if (const VkResult result =
			vkCreateDescriptorPool(m_context.getLogicalDevice(), &pool_info, nullptr, &m_descriptorPool);
		result != VK_SUCCESS) {
		throw render::vulkan::VulkanError("failed to create ImGui descriptor pool", result);
	}
}

void ImGuiGui::init() {
	ImGui_ImplGlfw_InitForVulkan(static_cast<GLFWwindow*>(m_window.getHandle()), true);

	ImGui_ImplVulkan_InitInfo init_info    = {};
	init_info.ApiVersion                   = render::vulkan::vulkanApiVersion;
	init_info.Instance                     = m_context.getInstance();
	init_info.PhysicalDevice               = m_context.getPhysicalDevice();
	init_info.Device                       = m_context.getLogicalDevice();
	init_info.QueueFamily                  = m_context.getQueueFamilies().graphics;
	init_info.Queue                        = m_context.getGraphicsQueue();
	init_info.PipelineCache                = VK_NULL_HANDLE;
	init_info.DescriptorPool               = m_descriptorPool;
	init_info.PipelineInfoMain.RenderPass  = m_swapchain.getRenderPass();
	init_info.PipelineInfoMain.Subpass     = 0;
	init_info.PipelineInfoMain.MSAASamples = m_context.getMsaaSamples();
	init_info.Allocator                    = nullptr;
	init_info.MinImageCount                = 2;
	init_info.ImageCount                   = static_cast<uint32_t>(m_swapchain.getImageCount());
	init_info.CheckVkResultFn              = nullptr;

	ImGui_ImplVulkan_Init(&init_info);
	m_isInitialized = true;
}

ImGuiGui::ImGuiGui(render::vulkan::VulkanContext& context, render::vulkan::VulkanSwapchain& swapchain,
				   platform::window::IWindow& window) :
	m_context(context), m_swapchain(swapchain), m_window(window), m_imguiContext(ImGui::CreateContext()) {
	ImGui::SetCurrentContext(m_imguiContext);
	createDescriptorPool();
	init();
}

ImGuiGui::~ImGuiGui() {
	vkDeviceWaitIdle(m_context.getLogicalDevice());
	if (m_isInitialized) {
		ImGui_ImplVulkan_Shutdown();
		ImGui_ImplGlfw_Shutdown();
	}
	if (m_descriptorPool != VK_NULL_HANDLE)
		vkDestroyDescriptorPool(m_context.getLogicalDevice(), m_descriptorPool, nullptr);
	ImGui::DestroyContext(m_imguiContext);
}

void ImGuiGui::beginFrame() {
	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
}

void ImGuiGui::endFrame() {
	ImGui::Render();
}

bool ImGuiGui::beginWindow(const std::string& name, bool* open) {
	++m_windowDepth;
	return ImGui::Begin(name.c_str(), open,
						ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize);
}

void ImGuiGui::endWindow() {
	if (m_windowDepth > 0)
		--m_windowDepth;
	ImGui::End();
}

bool ImGuiGui::beginSection(const std::string& name, [[maybe_unused]] bool* open) {
	ImGui::PushID(name.c_str());

	return ImGui::CollapsingHeader(name.c_str(), ImGuiTreeNodeFlags_CollapsingHeader);
}

void ImGuiGui::endSection() {
	ImGui::PopID();
}

void ImGuiGui::text(const std::string& value) {
	ImGui::Text("%s", value.c_str());
}

bool ImGuiGui::button(const std::string& label) {
	return ImGui::Button(label.c_str());
}

void ImGuiGui::separator() {
	ImGui::Separator();
}

void ImGuiGui::plotLines(const std::string& label, const std::span<const float> values, const float scaleMax) {
	ImGui::PlotLines(label.c_str(), values.data(), static_cast<int>(values.size()), 0, nullptr, 0.0f, scaleMax,
					 ImVec2(240.0f, 60.0f));
}

void ImGuiGui::render(VkCommandBuffer commandBuffer) {
	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);
}

bool ImGuiGui::wantsMouseCapture() const {
	return ImGui::GetIO().WantCaptureMouse;
}

bool ImGuiGui::wantsKeyboardCapture() const {
	return ImGui::GetIO().WantCaptureKeyboard;
}
