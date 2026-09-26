#include "ComponentDescriber.hpp"

using namespace ecs;

std::vector<ComponentDescription> ComponentDescriber::describe(const Registry& registry, const Entity entity) const {
	std::vector<ComponentDescription> descriptions;

	if (!registry.valid(entity))
		return descriptions;

	for (const auto& [name, describeComponent]: m_describers) {
		if (std::optional<std::string> details = describeComponent(registry, entity))
			descriptions.push_back({.name = name, .details = std::move(*details)});
	}
	return descriptions;
}
