#include "SystemManager.hpp"

using namespace ecs;

void SystemManager::unregisterEntity(const Entity& entity) const {
	for (const auto& [type, system]: m_systems)
		system->unregisterEntity(entity);
}

void SystemManager::onWorldReady() {
	m_dispatcher.emit<WorldReadyEvent>(WorldReadyEvent{});
}

void SystemManager::onRender(const float aspectRatio, const double time) {
	m_dispatcher.emit<RenderEvent>(RenderEvent{aspectRatio, time});
}

void SystemManager::onTextDraw(render::IRenderer& renderer) {
	m_dispatcher.emit<TextDrawEvent>(TextDrawEvent{&renderer});
}

void SystemManager::onRendererDraw(render::IRenderer& renderer) {
	m_dispatcher.emit<RendererDrawEvent>(RendererDrawEvent{&renderer});
}

void SystemManager::onRendererFrame(render::IRenderer& renderer) {
	m_dispatcher.emit<RendererFrameEvent>(RendererFrameEvent{&renderer});
}

void SystemManager::onSimulate(const float deltaTime, const float time) {
	m_dispatcher.emit<SimulateEvent>(SimulateEvent{deltaTime, time});
}

Dispatcher& SystemManager::getDispatcher() {
	return m_dispatcher;
}
