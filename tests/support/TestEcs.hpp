#pragma once

#include <string>
#include <vector>

#include "ecs/World.hpp"
#include "ecs/entity/EntityHandle.hpp"
#include "ecs/system/ASystem.hpp"
#include "ecs/system/DispatcherEvents.hpp"
#include "game/block/BlockData.hpp"

namespace test {

struct Health : public ecs::Component<Health> {
	int	points = 100;

	Health() : Component<Health>("Health") {}
	explicit Health(int points) : Health() { this->points = points; }
};

struct Armor : public ecs::Component<Armor> {
	int	rating = 0;

	Armor() : Component<Armor>("Armor") {}
	explicit Armor(int rating) : Armor() { this->rating = rating; }
};

class HealthSystem : public ecs::ASystem {
public:
	std::vector<std::string>	receivedEvents;
	float						lastDeltaTime = 0.0f;
	float						lastAspectRatio = 0.0f;
	render::IRenderer*			lastRenderer = nullptr;

	HealthSystem() : ASystem(ecs::Dependencies()) {
		m_dependencies.addDependency<Health>();
	}

	void	bindEvents(ecs::Dispatcher& dispatcher) override {
		dispatcher.subscribe<ecs::WorldReadyEvent>(this, &HealthSystem::onWorldReady);
		dispatcher.subscribe<ecs::RenderEvent>(this, &HealthSystem::onRender);
		dispatcher.subscribe<ecs::TextDrawEvent>(this, &HealthSystem::onTextDraw);
		dispatcher.subscribe<ecs::RendererDrawEvent>(this, &HealthSystem::onRendererDraw);
		dispatcher.subscribe<ecs::RendererFrameEvent>(this, &HealthSystem::onRendererFrame);
		dispatcher.subscribe<ecs::SimulateEvent>(this, &HealthSystem::onSimulate);
	}

	const std::vector<ecs::Entity>&	entities() const { return m_entities; }

private:
	void	onWorldReady(const ecs::WorldReadyEvent& event) { receivedEvents.push_back(event.getName()); }
	void	onRender(const ecs::RenderEvent& event) {
		receivedEvents.push_back(event.getName());
		lastAspectRatio = event.aspectRatio;
	}
	void	onTextDraw(const ecs::TextDrawEvent& event) {
		receivedEvents.push_back(event.getName());
		lastRenderer = event.renderer;
	}
	void	onRendererDraw(const ecs::RendererDrawEvent& event) {
		receivedEvents.push_back(event.getName());
		lastRenderer = event.renderer;
	}
	void	onRendererFrame(const ecs::RendererFrameEvent& event) {
		receivedEvents.push_back(event.getName());
		lastRenderer = event.renderer;
	}
	void	onSimulate(const ecs::SimulateEvent& event) {
		receivedEvents.push_back(event.getName());
		lastDeltaTime = event.deltaTime;
	}
};

struct TestWorld {
	game::block::BlockDatas	blockDatas{assets::MeshData{}, assets::TextureData{}};
	ecs::World				world{blockDatas};

	template <typename SystemType, typename... Args>
	SystemType&	addSystem(Args&... args) {
		world.createSystem<SystemType>(args...);
		return *world.getSystemManager().getSystem<SystemType>();
	}

	ecs::EntityHandle	createEntity() {
		return world.createEntity();
	}

	ecs::Dispatcher&	dispatcher() {
		return world.getSystemManager().getDispatcher();
	}

	void	simulate(float deltaTime, float time) {
		world.getSystemManager().onSimulate(deltaTime, time);
	}
};
}
