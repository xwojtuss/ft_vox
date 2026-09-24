#pragma once

#include "../ASystem.hpp"
#include "../DispatcherEvents.hpp"

namespace ecs {
	class RenderSystem : public ASystem {
	public:
		RenderSystem();

		void onRendererDraw(const RendererDrawEvent& event) const;
		void bindEvents(Dispatcher& dispatcher) override;
	};
}
