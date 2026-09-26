#pragma once

#include "../System.hpp"
#include "../DispatcherEvents.hpp"
#include "../../component/Components.hpp"

namespace ecs {
	class RenderSystem : public System<const component::Transform, const component::Mesh> {
	public:
		void onRendererDraw(const RendererDrawEvent& event) const;
		void bindEvents(Dispatcher& dispatcher) override;
	};
}
