#include "RenderSystem.hpp"
#include "../../../render/IRenderer.hpp"

using namespace ecs;

void RenderSystem::onRendererDraw(const RendererDrawEvent& event) const {
	for (auto&& [entity, transform, mesh]: entities()) {
		const auto* texture = m_world->getEntity(entity).tryGet<component::Texture>();
		event.renderer->drawMesh(mesh, texture, transform);
	}
}

void RenderSystem::bindEvents(Dispatcher& dispatcher) {
	dispatcher.subscribe<RendererDrawEvent>(this, &RenderSystem::onRendererDraw);
}
