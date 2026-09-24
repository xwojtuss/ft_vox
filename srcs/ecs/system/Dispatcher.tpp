#pragma once

namespace ecs {
	template<typename Event, typename SystemType>
	void Dispatcher::subscribe(SystemType* instance, void (SystemType::*method)(const Event&)) {
		auto& listeners = m_listeners[typeid(Event)];

		listeners.emplace_back([instance, method](const void* event) {
				(instance->*method)(*static_cast<const Event*>(event));
			}
		);
	}

	template<typename Event, typename SystemType>
	void Dispatcher::subscribe(const SystemType* instance, void (SystemType::*method)(const Event&) const) {
		m_listeners[typeid(Event)].emplace_back([instance, method](const void* event) {
			(instance->*method)(*static_cast<const Event*>(event));
		});
	}

	template<typename Event>
	void Dispatcher::emit(const Event& event) {
		m_eventRuntimes[event.getName()] = std::chrono::duration<float>::zero();

		auto it = m_listeners.find(typeid(Event));
		if (it == m_listeners.end())
			return;

		auto startTime = std::chrono::system_clock::now();
		for (auto& listener: it->second)
			listener(&event);
		auto endTime                     = std::chrono::system_clock::now();
		m_eventRuntimes[event.getName()] = std::chrono::duration<float>(endTime - startTime);
	}
}
