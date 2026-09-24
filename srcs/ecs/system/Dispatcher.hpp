#pragma once

#include <chrono>
#include <unordered_map>
#include <string>
#include <vector>
#include <typeindex>
#include <functional>

namespace ecs {
	using Listener = std::function<void(const void*)>;

	class Dispatcher {
	private:
		std::unordered_map<std::type_index, std::vector<Listener>>    m_listeners;
		std::unordered_map<std::string, std::chrono::duration<float>> m_eventRuntimes;

	public:
		template<typename Event, typename SystemType>
		void subscribe(SystemType* instance, void (SystemType::*method)(const Event&));

		template<typename Event, typename SystemType>
		void subscribe(const SystemType* instance, void (SystemType::*method)(const Event&) const);

		template<typename Event>
		void emit(const Event& event);

		[[nodiscard]] std::unordered_map<std::string, std::chrono::duration<float>> getEventRuntimes() const;
	};
}

#include "Dispatcher.tpp"
