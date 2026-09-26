#include "Dispatcher.hpp"

using namespace ecs;

std::unordered_map<std::string, std::chrono::duration<float>> Dispatcher::getEventRuntimes() const {
	return m_eventRuntimes;
}
