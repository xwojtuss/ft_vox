#pragma once

#include <span>
#include <string>
#include <vector>

#include "render/gui/IGui.hpp"

namespace test {
	class FakeGui : public render::gui::IGui {
	public:
		std::vector<std::string> calls;
		std::vector<std::string> openedWindows;
		std::vector<std::string> texts;
		bool                     capturingMouse = false;

		void beginFrame() override {
			calls.emplace_back("beginFrame");
		}
		void endFrame() override {
			calls.emplace_back("endFrame");
		}

		bool beginWindow(const std::string& name, bool*) override {
			openedWindows.push_back(name);
			return true;
		}

		bool beginSection(const std::string&, bool*) override {
			return true;
		}
		void text(const std::string& value) override {
			texts.push_back(value);
		}
		[[nodiscard]] bool wantsMouseCapture() const override {
			return capturingMouse;
		}

		void endWindow() override {
		}

		void endSection() override {
		}

		bool button(const std::string&) override {
			return false;
		}

		void separator() override {
		}

		void plotLines(const std::string&, std::span<const float>, float) override {
		}

		void render(VkCommandBuffer) override {
		}

		[[nodiscard]] bool wantsKeyboardCapture() const override {
			return false;
		}
	};
}
