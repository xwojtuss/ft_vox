#pragma once

#include <ostream>
#include <string>
#include <utility>

namespace ecs {
	struct ComponentId {
		static int id;
	};

	struct IComponent {
	protected:
		std::string m_name;

		explicit IComponent(std::string name) : m_name(std::move(name)) {
		}

	public:
		virtual ~IComponent() = default;

		virtual std::ostream& print(std::ostream& os) const = 0;

		[[nodiscard]] const std::string& getName() const;

		friend std::ostream& operator<<(std::ostream& os, const IComponent& component) {
			return component.print(os);
		}
	};

	template<typename ComponentType>
	struct Component : public IComponent {
	protected:
		explicit Component(const std::string& name) : IComponent(name) {
		}

	public:
		static int getId();

		std::ostream& print(std::ostream& os) const override;
	};
}

#include "Component.tpp"
