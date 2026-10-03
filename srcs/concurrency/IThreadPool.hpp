#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>

namespace concurrency {
	using Task     = std::function<void()>;
	using Priority = std::uint8_t;

	constexpr Priority lowestPriority  = 0;
	constexpr Priority highestPriority = 255;

	class IThreadPool {
	public:
		virtual ~IThreadPool() = default;

		virtual void                      submit(Priority priority, Task task) = 0;
		virtual void                      waitUntilIdle()                      = 0;
		[[nodiscard]] virtual std::size_t workerCount() const                  = 0;
		[[nodiscard]] virtual std::size_t unfinishedTaskCount() const          = 0;
	};

	[[nodiscard]] std::size_t                  defaultWorkerCount();
	[[nodiscard]] std::unique_ptr<IThreadPool> createThreadPool(std::size_t workerCount);
}
