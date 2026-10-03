#include <BS_thread_pool.hpp>

#include <algorithm>
#include <exception>
#include <thread>

#include "concurrency/IThreadPool.hpp"
#include "log/Log.hpp"

namespace {
	constexpr std::size_t reservedCores = 2;

	constexpr int priorityOffset = 128;

	class BsThreadPool final : public concurrency::IThreadPool {
	private:
		BS::priority_thread_pool m_pool;

	public:
		explicit BsThreadPool(const std::size_t workerCount) : m_pool(workerCount) {
		}

		void submit(const concurrency::Priority priority, concurrency::Task task) override {
			m_pool.detach_task(
				[task = std::move(task)] mutable {
					try {
						task();
					} catch (const std::exception& exception) {
						logging::app().error("a background task failed: {}", exception.what());
					}
				},
				static_cast<BS::priority_t>(static_cast<int>(priority) - priorityOffset));
		}

		void waitUntilIdle() override {
			m_pool.wait();
		}

		[[nodiscard]] std::size_t workerCount() const override {
			return m_pool.get_thread_count();
		}

		[[nodiscard]] std::size_t unfinishedTaskCount() const override {
			return m_pool.get_tasks_total();
		}
	};
}

std::size_t concurrency::defaultWorkerCount() {
	const std::size_t cores = std::thread::hardware_concurrency();
	return cores > reservedCores ? cores - reservedCores : 1;
}

std::unique_ptr<concurrency::IThreadPool> concurrency::createThreadPool(const std::size_t workerCount) {
	return std::make_unique<BsThreadPool>(std::max<std::size_t>(workerCount, 1));
}
