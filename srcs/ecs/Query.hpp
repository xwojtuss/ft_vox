#pragma once

#include <cstddef>
#include <utility>

#include "ecs/Storage.hpp"

namespace ecs {
	/**
	 * Every entity that has all of Components, iterated as [entity, components...].
	 * Empty components are required but not part of the iterated tuple.
	 */
	template<typename... Components>
	class Query {
	private:
		using View = decltype(std::declval<Storage&>().view<Components...>());

		View m_view{};

	public:
		explicit Query(Storage& storage) : m_view(storage.view<Components...>()) {
		}

		[[nodiscard]] auto begin() const {
			return m_view.each().begin();
		}
		[[nodiscard]] auto end() const {
			return m_view.each().end();
		}

		[[nodiscard]] bool contains(const Entity entity) const {
			return m_view.contains(entity);
		}

		[[nodiscard]] std::size_t count() const {
			std::size_t count = 0;
			for (auto it = begin(); it != end(); ++it)
				++count;
			return count;
		}

		[[nodiscard]] bool empty() const {
			return begin() == end();
		}
	};
}
