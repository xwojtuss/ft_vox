#pragma once

namespace ecs {
	template<typename ComponentType>
	int Component<ComponentType>::getId() {
		static const int id = ComponentId::id++;
		return id;
	}

	template<typename ComponentType>
	std::ostream& Component<ComponentType>::print(std::ostream& os) const {
		return os << "Component";
	}
}
