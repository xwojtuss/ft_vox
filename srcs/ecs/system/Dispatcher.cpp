#include "Dispatcher.hpp"

using namespace ecs;

std::unordered_map<std::string, std::chrono::duration<float>> Dispatcher::getEventRuntimes() const {
	std::unordered_map<std::string, std::chrono::duration<float>> runtimes;

	for (const auto& [typeName, runtime]: m_eventRuntimes) {
		runtimes[typeName] = runtime;
	}
	return runtimes;
}
