#pragma once

#include <format>
#include <string>
#include <string_view>
#include <vector>

#include "ecs/World.hpp"
#include "ecs/entity/EntityHandle.hpp"
#include "ecs/system/System.hpp"
#include "ecs/system/DispatcherEvents.hpp"
#include "game/block/BlockData.hpp"

namespace test {
	struct Health {
		int points = 100;
	};

	struct Armor {
		int rating = 0;
	};

	class HealthSystem : public ecs::System<Health> {
	public:
		std::vector<std::string> receivedEvents;
		float                    lastDeltaTime   = 0.0f;
		float                    lastAspectRatio = 0.0f;
		render::IRenderer*       lastRenderer    = nullptr;

		void bindEvents(ecs::Dispatcher& dispatcher) override {
			dispatcher.subscribe<ecs::WorldReadyEvent>(this, &HealthSystem::onWorldReady);
			dispatcher.subscribe<ecs::RenderEvent>(this, &HealthSystem::onRender);
			dispatcher.subscribe<ecs::TextDrawEvent>(this, &HealthSystem::onTextDraw);
			dispatcher.subscribe<ecs::RendererDrawEvent>(this, &HealthSystem::onRendererDraw);
			dispatcher.subscribe<ecs::RendererFrameEvent>(this, &HealthSystem::onRendererFrame);
			dispatcher.subscribe<ecs::SimulateEvent>(this, &HealthSystem::onSimulate);
		}

	private:
		void onWorldReady(const ecs::WorldReadyEvent& event) { receivedEvents.push_back(event.getName()); }

		void onRender(const ecs::RenderEvent& event) {
			receivedEvents.push_back(event.getName());
			lastAspectRatio = event.aspectRatio;
		}

		void onTextDraw(const ecs::TextDrawEvent& event) {
			receivedEvents.push_back(event.getName());
			lastRenderer = event.renderer;
		}

		void onRendererDraw(const ecs::RendererDrawEvent& event) {
			receivedEvents.push_back(event.getName());
			lastRenderer = event.renderer;
		}

		void onRendererFrame(const ecs::RendererFrameEvent& event) {
			receivedEvents.push_back(event.getName());
			lastRenderer = event.renderer;
		}

		void onSimulate(const ecs::SimulateEvent& event) {
			receivedEvents.push_back(event.getName());
			lastDeltaTime = event.deltaTime;
		}
	};

	struct TestWorld {
		game::block::BlockDatas blockDatas{assets::MeshData{}, assets::TextureData{}};
		ecs::World              world{blockDatas};

		TestWorld() {
			world.describeComponentAs<Health>("Health");
			world.describeComponentAs<Armor>("Armor");
		}

		template<typename SystemType, typename... Args>
		SystemType& addSystem(Args&... args) {
			world.createSystem<SystemType>(args...);
			return *world.getSystemManager().getSystem<SystemType>();
		}

		ecs::EntityHandle createEntity() {
			return world.createEntity();
		}

		ecs::Dispatcher& dispatcher() {
			return world.getSystemManager().getDispatcher();
		}

		void simulate(float deltaTime, float time) {
			world.getSystemManager().onSimulate(deltaTime, time);
		}
	};
}

template<>
struct std::formatter<test::Health> : std::formatter<std::string_view> {
	auto format(const test::Health& health, std::format_context& context) const {
		return std::format_to(context.out(), "Points: {}", health.points);
	}
};

template<>
struct std::formatter<test::Armor> : std::formatter<std::string_view> {
	auto format(const test::Armor& armor, std::format_context& context) const {
		return std::format_to(context.out(), "Rating: {}", armor.rating);
	}
};
