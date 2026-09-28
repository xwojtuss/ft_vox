#pragma once

#include "ecs/system/System.hpp"
#include "ecs/system/DispatcherEvents.hpp"
#include "ecs/component/Components.hpp"

namespace ecs {
	class CameraSystem : public System<const component::Transform, component::Camera> {
	public:
		void onRender(const RenderEvent& event) const;
		void onRendererFrame(const RendererFrameEvent& event) const;
		void bindEvents(Dispatcher& dispatcher) override;
	};
}
