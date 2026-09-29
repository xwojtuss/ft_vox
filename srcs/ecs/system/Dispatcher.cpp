#include "ecs/system/Dispatcher.hpp"

using namespace ecs;

const std::unordered_map<std::string_view, std::chrono::duration<float>>& Dispatcher::getEventRuntimes() const {
	return m_eventRuntimes;
}
