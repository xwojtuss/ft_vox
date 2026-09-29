#pragma once

#include <utility>

namespace ecs {
	template<typename ComponentType>
	void ComponentDescriber::add(const std::string_view name) {
		m_describers.push_back(
			{.name     = std::string(name),
			 .describe = [](const Storage& storage, const Entity entity) -> std::optional<std::string> {
				 if (const ComponentType* component = storage.try_get<ComponentType>(entity))
					 return std::format("{}", *component);
				 return std::nullopt;
			 }});
	}
}
