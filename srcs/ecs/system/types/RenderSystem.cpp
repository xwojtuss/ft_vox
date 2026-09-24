#include "RenderSystem.hpp"
#include "../../component/Components.hpp"
#include "../../../render/IRenderer.hpp"
#include "../../World.hpp"

using namespace ecs;

RenderSystem::RenderSystem() : ASystem(Dependencies()) {
	m_dependencies.addDependency<component::Transform>();
	m_dependencies.addDependency<component::Mesh>();
}

void RenderSystem::onRendererDraw(const RendererDrawEvent& event) const {
	for (const Entity& entity: m_entities) {
		const component::Transform* transform = m_world->getComponentManager<component::Transform>().
														getComponent(entity);
		const component::Mesh*    mesh    = m_world->getComponentManager<component::Mesh>().getComponent(entity);
		const component::Texture* texture = m_world->getComponentManager<component::Texture>().getComponent(entity);

		if (!transform || !mesh)
			continue;

		event.renderer->drawMesh(*mesh, texture, *transform);
	}
}

void RenderSystem::bindEvents(Dispatcher& dispatcher) {
	dispatcher.subscribe<RendererDrawEvent>(this, &RenderSystem::onRendererDraw);
}
