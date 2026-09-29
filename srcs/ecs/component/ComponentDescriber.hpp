#pragma once

#include <format>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "ecs/Storage.hpp"

namespace ecs {
	struct ComponentDescription {
		std::string name;
		std::string details;
	};

	class ComponentDescriber {
	private:
		struct Describer {
			std::string                                                       name;
			std::function<std::optional<std::string>(const Storage&, Entity)> describe;
		};

		std::vector<Describer> m_describers;

	public:
		template<typename ComponentType>
		void add(std::string_view name);

		[[nodiscard]] std::vector<ComponentDescription> describe(const Storage& storage, Entity entity) const;
	};
}

#include "ecs/component/ComponentDescriber.tpp"
