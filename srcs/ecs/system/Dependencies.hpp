#pragma once

#include <bitset>

namespace ecs {
	using DependencyMask = std::bitset<32>;

	struct Dependencies {
		DependencyMask mask;

		Dependencies();

		template<typename ComponentType>
		void addDependency();

		template<typename ComponentType>
		void removeDependency();

		template<typename ComponentType>
		[[nodiscard]] bool includes() const;

		[[nodiscard]] bool matches(const Dependencies& other) const;
	};
}

#include "Dependencies.tpp"
