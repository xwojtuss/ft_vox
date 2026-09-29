#include "ecs/component/ComponentDescriber.hpp"

using namespace ecs;

std::vector<ComponentDescription> ComponentDescriber::describe(const Storage& storage, const Entity entity) const {
	std::vector<ComponentDescription> descriptions;

	if (!storage.valid(entity))
		return descriptions;

	for (const auto& [name, describeComponent]: m_describers) {
		if (std::optional<std::string> details = describeComponent(storage, entity))
			descriptions.push_back({.name = name, .details = std::move(*details)});
	}
	return descriptions;
}
