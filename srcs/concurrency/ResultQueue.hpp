#pragma once

#include <mutex>
#include <vector>

namespace concurrency {
	template<typename T>
	class ResultQueue {
	private:
		std::mutex     m_mutex;
		std::vector<T> m_items;

	public:
		void                         push(T item);
		[[nodiscard]] std::vector<T> drain();
	};
}

#include "concurrency/ResultQueue.tpp"
