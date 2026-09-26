#include "CameraSystem.hpp"
#include "../../../scene/WorldInfo.hpp"
#include "../../../render/IRenderer.hpp"

using namespace ecs;

void CameraSystem::onRender(const RenderEvent& event) const {
	for (auto&& [entity, transform, camera]: entities()) {
		camera.projection = glm::perspective(glm::radians(camera.fov), event.aspectRatio, camera.nearPlane,
											camera.farPlane);
		camera.view = glm::lookAt(transform.position, transform.position + transform.forward(), scene::worldinfo::up);
	}
}

void CameraSystem::onRendererFrame(const RendererFrameEvent& event) const {
	for (auto&& [entity, transform, camera]: entities())
		event.renderer->updateCamera(camera);
}

void CameraSystem::bindEvents(Dispatcher& dispatcher) {
	dispatcher.subscribe(this, &CameraSystem::onRender);
	dispatcher.subscribe(this, &CameraSystem::onRendererFrame);
}
