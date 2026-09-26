#pragma once

#include <format>
#include <functional>
#include <optional>
#include <string>
#include <vector>

#include "../Registry.hpp"

namespace ecs {
	struct ComponentDescription {
		std::string name;
		std::string details;
	};

	class ComponentDescriber {
	private:
		struct Describer {
			std::string                                                         name;
			std::function<std::optional<std::string>(const Registry &, Entity)> describe;
		};

		std::vector<Describer> m_describers;

	public:
		template<typename ComponentType>
		void add(std::string name);

		[[nodiscard]] std::vector<ComponentDescription> describe(const Registry& registry, Entity entity) const;
	};
}

#include "ComponentDescriber.tpp"
