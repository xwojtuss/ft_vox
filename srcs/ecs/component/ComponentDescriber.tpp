#pragma once

#include <utility>

namespace ecs {
	template<typename ComponentType>
	void ComponentDescriber::add(std::string name) {
		m_describers.push_back({
			.name     = std::move(name),
			.describe = [](const Registry& registry, const Entity entity) -> std::optional<std::string> {
				if (const ComponentType* component = registry.try_get<ComponentType>(entity))
					return std::format("{}", *component);
				return std::nullopt;
			}
		});
	}
}
