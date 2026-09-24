#pragma once

#include "../ASystem.hpp"
#include "../DispatcherEvents.hpp"

namespace ecs {
	class World;

	class CameraSystem : public ASystem {
	public:
		CameraSystem();

		void onRender(const RenderEvent& event) const;
		void onRendererFrame(const RendererFrameEvent& event) const;
		void bindEvents(Dispatcher& dispatcher) override;
	};
}
