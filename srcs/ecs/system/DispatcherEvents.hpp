#pragma once

#include <utility>

#include "../../render/input/InputTypes.hpp"

namespace render {
	class IRenderer;
}

namespace ecs {
	struct DispatchEvent {
	protected:
		std::string m_name;

		explicit DispatchEvent(std::string name) : m_name(std::move(name)) {
		}

	public:
		virtual ~DispatchEvent() = default;

		[[nodiscard]] const std::string& getName() const { return m_name; }
	};

	struct RenderEvent : public DispatchEvent {
		float  aspectRatio;
		double time;

		RenderEvent(const float aspectRatio, const double time) : DispatchEvent("RenderEvent"),
																aspectRatio(aspectRatio), time(time) {
		}
	};

	struct TextDrawEvent : public DispatchEvent {
		render::IRenderer* renderer;

		explicit TextDrawEvent(render::IRenderer* renderer) : DispatchEvent("TextDrawEvent"), renderer(renderer) {
		}
	};

	struct RendererDrawEvent : public DispatchEvent {
		render::IRenderer* renderer;

		explicit RendererDrawEvent(render::IRenderer* renderer) : DispatchEvent("RendererDrawEvent"),
																renderer(renderer) {
		}
	};

	struct RendererFrameEvent : public DispatchEvent {
		render::IRenderer* renderer;

		explicit RendererFrameEvent(render::IRenderer* renderer) : DispatchEvent("RendererFrameEvent"),
																	renderer(renderer) {
		}
	};

	struct InputEvent : public DispatchEvent {
		float                       deltaTime{0};
		ecs::Entity                 source{-1};
		render::input::InputCommand command;

		InputEvent() : DispatchEvent("InputEvent") {
		}
	};

	struct WorldReadyEvent : public DispatchEvent {
		WorldReadyEvent() : DispatchEvent("WorldReadyEvent") {
		}
	};

	struct SimulateEvent : public DispatchEvent {
		float deltaTime;
		float time;

		SimulateEvent(const float deltaTime, const float time) : DispatchEvent("SimulateEvent"), deltaTime(deltaTime),
																time(time) {
		}
	};

	struct PlayerMoveEvent : public DispatchEvent {
		glm::vec3 previousPosition;
		glm::vec3 currentPosition;

		PlayerMoveEvent(const glm::vec3 previousPosition, const glm::vec3 currentPosition) : DispatchEvent(
				"PlayerMoveEvent"), previousPosition(previousPosition), currentPosition(currentPosition) {
		}
	};
}
