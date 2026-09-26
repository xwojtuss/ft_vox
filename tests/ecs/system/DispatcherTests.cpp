#include <catch2/catch_test_macros.hpp>

#include <string>
#include <vector>

#include "ecs/entity/Entity.hpp"
#include "ecs/system/Dispatcher.hpp"
#include "ecs/system/DispatcherEvents.hpp"

namespace {
	struct EventRecorder {
		std::string               name;
		std::vector<std::string>* calls;
		float                     lastDeltaTime = 0.0f;

		void onSimulate(const ecs::SimulateEvent& event) {
			calls->push_back(name);
			lastDeltaTime = event.deltaTime;
		}

		void onRender(const ecs::RenderEvent&) const {
			calls->push_back(name + " rendered");
		}

		void onWorldReady(const ecs::WorldReadyEvent&) {
			calls->push_back(name + " ready");
		}
	};
}

SCENARIO("Events reach every subscriber of their type, in subscription order", "[ecs][dispatcher]") {
	GIVEN("two listeners subscribed to simulate events and one to world-ready events") {
		ecs::Dispatcher          dispatcher;
		std::vector<std::string> calls;
		EventRecorder            first{"first", &calls};
		EventRecorder            second{"second", &calls};
		EventRecorder            other{"other", &calls};

		dispatcher.subscribe<ecs::SimulateEvent>(&first, &EventRecorder::onSimulate);
		dispatcher.subscribe<ecs::SimulateEvent>(&second, &EventRecorder::onSimulate);
		dispatcher.subscribe<ecs::WorldReadyEvent>(&other, &EventRecorder::onWorldReady);

		WHEN("a simulate event is emitted") {
			dispatcher.emit(ecs::SimulateEvent(0.25f, 1.0f));

			THEN("both simulate listeners get it, first subscriber first") {
				REQUIRE(calls == std::vector<std::string>{"first", "second"});
			}
			AND_THEN("they receive the event's data") {
				REQUIRE(first.lastDeltaTime == 0.25f);
				REQUIRE(second.lastDeltaTime == 0.25f);
			}
		}

		WHEN("a world-ready event is emitted") {
			dispatcher.emit(ecs::WorldReadyEvent());

			THEN("only its own listener gets it") {
				REQUIRE(calls == std::vector<std::string>{"other ready"});
			}
		}
	}
}

SCENARIO("Handlers that do not change their system can be const", "[ecs][dispatcher]") {
	GIVEN("a listener whose render handler is a const member function") {
		ecs::Dispatcher          dispatcher;
		std::vector<std::string> calls;
		const EventRecorder      listener{"listener", &calls};
		dispatcher.subscribe<ecs::RenderEvent>(&listener, &EventRecorder::onRender);

		WHEN("a render event is emitted") {
			dispatcher.emit(ecs::RenderEvent(1.0f, 0.0));

			THEN("the const handler receives it") {
				REQUIRE(calls == std::vector<std::string>{"listener rendered"});
			}
		}
	}
}

SCENARIO("The dispatcher measures how long each event took", "[ecs][dispatcher]") {
	GIVEN("a dispatcher with one simulate listener") {
		ecs::Dispatcher          dispatcher;
		std::vector<std::string> calls;
		EventRecorder            listener{"listener", &calls};
		dispatcher.subscribe<ecs::SimulateEvent>(&listener, &EventRecorder::onSimulate);

		THEN("nothing is measured before any event") {
			REQUIRE(dispatcher.getEventRuntimes().empty());
		}

		WHEN("a simulate event and an event nobody listens to are emitted") {
			dispatcher.emit(ecs::SimulateEvent(0.1f, 0.0f));
			dispatcher.emit(ecs::RenderEvent(1.5f, 0.0));

			THEN("both appear in the runtimes by name") {
				const auto runtimes = dispatcher.getEventRuntimes();

				REQUIRE(runtimes.size() == 2);
				REQUIRE(runtimes.count("SimulateEvent") == 1);
				REQUIRE(runtimes.at("SimulateEvent").count() >= 0.0f);
			}
			AND_THEN("the event nobody listens to took no time") {
				REQUIRE(dispatcher.getEventRuntimes().at("RenderEvent").count() == 0.0f);
			}
		}
	}
}

SCENARIO("Every event type has a readable name", "[ecs][dispatcher]") {
	THEN("each event is named after its type") {
		REQUIRE(ecs::RenderEvent(1.0f, 0.0).getName() == "RenderEvent");
		REQUIRE(ecs::TextDrawEvent(nullptr).getName() == "TextDrawEvent");
		REQUIRE(ecs::RendererDrawEvent(nullptr).getName() == "RendererDrawEvent");
		REQUIRE(ecs::RendererFrameEvent(nullptr).getName() == "RendererFrameEvent");
		REQUIRE(ecs::InputEvent().getName() == "InputEvent");
		REQUIRE(ecs::WorldReadyEvent().getName() == "WorldReadyEvent");
		REQUIRE(ecs::SimulateEvent(0.0f, 0.0f).getName() == "SimulateEvent");
		REQUIRE(ecs::PlayerMoveEvent(glm::vec3(0.0f), glm::vec3(1.0f)).getName() == "PlayerMoveEvent");
	}
	AND_THEN("an input event starts with no source entity and no elapsed time") {
		const ecs::InputEvent event;
		REQUIRE(event.source == ecs::nullEntity);
		REQUIRE(event.deltaTime == 0.0f);
	}
}
