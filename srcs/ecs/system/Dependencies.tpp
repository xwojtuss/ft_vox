#pragma once
#include "component/Component.hpp"

namespace ecs {
	template<typename ComponentType>
	void Dependencies::addDependency() {
		mask.set(Component<ComponentType>::getId());
	}

	template<typename ComponentType>
	void Dependencies::removeDependency() {
		mask.reset(Component<ComponentType>::getId());
	}

	template<typename ComponentType>
	bool Dependencies::includes() const {
		return mask.test(Component<ComponentType>::getId());
	}
}
