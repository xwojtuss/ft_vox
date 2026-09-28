#pragma once

#include "ecs/system/System.hpp"
#include "ecs/system/DispatcherEvents.hpp"
#include "ecs/component/Components.hpp"

namespace ecs {
	class RenderSystem : public System<const component::Transform, const component::Mesh> {
	public:
		void onRendererDraw(const RendererDrawEvent& event) const;
		void bindEvents(Dispatcher& dispatcher) override;
	};
}
