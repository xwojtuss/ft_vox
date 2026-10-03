#pragma once

#include <algorithm>
#include <cstddef>
#include <vector>

#include "concurrency/IThreadPool.hpp"

namespace test {
	class ManualThreadPool : public concurrency::IThreadPool {
	private:
		struct Entry {
			concurrency::Priority priority;
			std::size_t           sequence;
			concurrency::Task     task;
		};

		std::vector<Entry> m_queue;
		std::size_t        m_nextSequence = 0;

	public:
		void submit(const concurrency::Priority priority, concurrency::Task task) override {
			m_queue.push_back({.priority = priority, .sequence = m_nextSequence++, .task = std::move(task)});
		}

		bool runNext() {
			if (m_queue.empty())
				return false;

			const auto              next = std::ranges::min_element(m_queue, [](const Entry& a, const Entry& b) {
                return a.priority != b.priority ? a.priority > b.priority : a.sequence < b.sequence;
            });
			const concurrency::Task task = std::move(next->task);
			m_queue.erase(next);
			task();
			return true;
		}

		void runAll() {
			while (runNext()) {
			}
		}

		void waitUntilIdle() override {
			runAll();
		}

		[[nodiscard]] std::size_t workerCount() const override {
			return 1;
		}

		[[nodiscard]] std::size_t unfinishedTaskCount() const override {
			return m_queue.size();
		}

		[[nodiscard]] std::size_t pending() const {
			return m_queue.size();
		}
	};
}
