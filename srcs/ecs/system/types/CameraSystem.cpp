#include "CameraSystem.hpp"
#include "../../../scene/WorldInfo.hpp"
#include "../../component/Components.hpp"
#include "../../../render/IRenderer.hpp"
#include "../../World.hpp"

using namespace ecs;

CameraSystem::CameraSystem() : ASystem(Dependencies()) {
	m_dependencies.addDependency<component::Transform>();
	m_dependencies.addDependency<component::Camera>();
}

void CameraSystem::onRender(const RenderEvent& event) const {
	for (const Entity& entity: m_entities) {
		const component::Transform* transform = m_world->getComponentManager<component::Transform>().
														getComponent(entity);
		component::Camera* camera = m_world->getComponentManager<component::Camera>().getComponent(entity);

		if (!transform || !camera)
			continue;

		camera->projection = glm::perspective(glm::radians(camera->fov), event.aspectRatio, 0.1f, 1e10f);
		camera->view       = glm::lookAt(transform->position, transform->position + transform->forward(),
									scene::worldinfo::up);
	}
}

void CameraSystem::onRendererFrame(const RendererFrameEvent& event) const {
	for (const Entity& entity: m_entities) {
		const component::Camera* camera = m_world->getComponentManager<component::Camera>().getComponent(entity);

		if (!camera)
			continue;

		event.renderer->updateCamera(*camera);
	}
}

void CameraSystem::bindEvents(Dispatcher& dispatcher) {
	dispatcher.subscribe(this, &CameraSystem::onRender);
	dispatcher.subscribe(this, &CameraSystem::onRendererFrame);
}
