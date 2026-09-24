#pragma once

#include <string>
#include <vulkan/vulkan.h>

namespace render::gui {
	class IGui {
	public:
		virtual ~IGui() = default;

		virtual void               beginFrame() = 0;
		virtual void               endFrame() = 0;
		virtual bool               beginWindow(const std::string& name, bool* open) = 0;
		virtual void               endWindow() = 0;
		virtual bool               beginSection(const std::string& name, bool* open) = 0;
		virtual void               endSection() = 0;
		virtual bool               button(const std::string& label) = 0;
		virtual void               text(const std::string& value) = 0;
		virtual void               separator() = 0;
		virtual void               render(VkCommandBuffer commandBuffer) = 0;
		[[nodiscard]] virtual bool wantsMouseCapture() const = 0;
		[[nodiscard]] virtual bool wantsKeyboardCapture() const = 0;
	};
}
