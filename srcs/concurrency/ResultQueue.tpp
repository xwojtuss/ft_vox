#pragma once

#include <utility>

namespace concurrency {
	template<typename T>
	void ResultQueue<T>::push(T item) {
		const std::scoped_lock lock(m_mutex);
		m_items.push_back(std::move(item));
	}

	template<typename T>
	std::vector<T> ResultQueue<T>::drain() {
		const std::scoped_lock lock(m_mutex);
		return std::exchange(m_items, {});
	}
}
