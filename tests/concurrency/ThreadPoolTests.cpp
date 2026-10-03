#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <mutex>
#include <thread>
#include <vector>

#include "concurrency/IThreadPool.hpp"

using namespace std::chrono_literals;

SCENARIO("A thread pool runs every submitted task", "[concurrency][thread-pool]") {
	GIVEN("a pool with four workers") {
		const auto pool = concurrency::createThreadPool(4);

		THEN("it reports four workers") {
			REQUIRE(pool->workerCount() == 4);
		}

		WHEN("a thousand tasks are submitted") {
			std::atomic<int> done = 0;
			for (int i = 0; i < 1000; ++i)
				pool->submit(concurrency::lowestPriority, [&done] { ++done; });
			pool->waitUntilIdle();

			THEN("all of them have run when the pool is idle") {
				REQUIRE(done == 1000);
				REQUIRE(pool->unfinishedTaskCount() == 0);
			}
		}

		WHEN("tasks run at the same time") {
			std::atomic<int> running    = 0;
			std::atomic<int> mostAtOnce = 0;
			for (int i = 0; i < 8; ++i) {
				pool->submit(concurrency::lowestPriority, [&] {
					const int now  = ++running;
					int       seen = mostAtOnce;
					while (now > seen && !mostAtOnce.compare_exchange_weak(seen, now)) {
					}
					std::this_thread::sleep_for(50ms);
					--running;
				});
			}
			pool->waitUntilIdle();

			THEN("more than one task ran at once") {
				REQUIRE(mostAtOnce > 1);
			}
		}
	}
}

SCENARIO("A thread pool starts urgent tasks first", "[concurrency][thread-pool]") {
	GIVEN("a pool with one worker that is kept busy") {
		const auto pool = concurrency::createThreadPool(1);

		std::atomic<bool> release = false;
		pool->submit(concurrency::highestPriority, [&release] {
			while (!release)
				std::this_thread::sleep_for(1ms);
		});

		WHEN("tasks with different priorities are queued behind it") {
			std::mutex       mutex;
			std::vector<int> order;
			const auto       record = [&](const int id) {
                const std::scoped_lock lock(mutex);
                order.push_back(id);
			};

			pool->submit(10, [&] { record(10); });
			pool->submit(200, [&] { record(200); });
			pool->submit(100, [&] { record(100); });
			pool->submit(100, [&] { record(101); });
			release = true;
			pool->waitUntilIdle();

			THEN("they run from the highest priority to the lowest") {
				REQUIRE(order.size() == 4);
				REQUIRE(order.front() == 200);
				REQUIRE(order.back() == 10);
			}
			AND_THEN("the two with equal priority run in the middle, in either order") {
				REQUIRE(std::ranges::count(order.begin() + 1, order.begin() + 3, 100) == 1);
				REQUIRE(std::ranges::count(order.begin() + 1, order.begin() + 3, 101) == 1);
			}
		}
	}
}

SCENARIO("A failing task does not stop the pool", "[concurrency][thread-pool]") {
	GIVEN("a pool and a task that throws") {
		const auto pool = concurrency::createThreadPool(2);
		pool->submit(concurrency::lowestPriority, [] { throw std::runtime_error("broken task"); });

		WHEN("another task is submitted afterwards") {
			std::atomic<bool> ran = false;
			pool->submit(concurrency::lowestPriority, [&ran] { ran = true; });
			pool->waitUntilIdle();

			THEN("it still runs") {
				REQUIRE(ran);
			}
		}
	}
}

SCENARIO("The default number of workers leaves cores free", "[concurrency][thread-pool]") {
	GIVEN("the machine's cores") {
		const std::size_t cores = std::thread::hardware_concurrency();

		THEN("the pool uses all but two, and at least one") {
			REQUIRE(concurrency::defaultWorkerCount() >= 1);
			if (cores > 2)
				REQUIRE(concurrency::defaultWorkerCount() == cores - 2);
		}
	}

	GIVEN("a request for zero workers") {
		THEN("the pool still gets one") {
			REQUIRE(concurrency::createThreadPool(0)->workerCount() == 1);
		}
	}
}
