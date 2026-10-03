#include <catch2/catch_test_macros.hpp>

#include <memory>
#include <thread>
#include <vector>

#include "concurrency/CancelToken.hpp"
#include "concurrency/ResultQueue.hpp"

namespace {
	concurrency::CancelToken copyOf(const concurrency::CancelToken& token) {
		return token;
	}
}

SCENARIO("A result queue hands over what was pushed, oldest first", "[concurrency][result-queue]") {
	GIVEN("a queue with three results") {
		concurrency::ResultQueue<int> queue;
		queue.push(1);
		queue.push(2);
		queue.push(3);

		THEN("draining returns them in order") {
			REQUIRE(queue.drain() == std::vector<int>{1, 2, 3});
		}
		AND_THEN("draining again returns nothing") {
			static_cast<void>(queue.drain());
			REQUIRE(queue.drain().empty());
		}
	}

	GIVEN("a queue of move-only results") {
		concurrency::ResultQueue<std::unique_ptr<int>> queue;
		queue.push(std::make_unique<int>(5));

		THEN("they can be drained") {
			const auto results = queue.drain();
			REQUIRE(results.size() == 1);
			REQUIRE(*results.front() == 5);
		}
	}
}

SCENARIO("Several threads can push results at the same time", "[concurrency][result-queue][threads]") {
	GIVEN("a queue and four threads that push a thousand results each") {
		concurrency::ResultQueue<int> queue;
		{
			std::vector<std::jthread> threads;
			threads.reserve(4);
			for (int thread = 0; thread < 4; ++thread) {
				threads.emplace_back([&queue] {
					for (int i = 0; i < 1000; ++i)
						queue.push(i);
				});
			}
		}

		THEN("all four thousand results arrive") {
			REQUIRE(queue.drain().size() == 4000);
		}
	}
}

SCENARIO("A cancel token is shared between its copies", "[concurrency][cancel-token]") {
	GIVEN("a token and a copy of it") {
		const concurrency::CancelToken token;
		const concurrency::CancelToken copy = copyOf(token);

		THEN("neither is cancelled at first") {
			REQUIRE_FALSE(token.isCancelled());
			REQUIRE_FALSE(copy.isCancelled());
		}

		WHEN("the original is cancelled") {
			token.cancel();

			THEN("the copy is cancelled too") {
				REQUIRE(copy.isCancelled());
			}
		}
	}

	GIVEN("two separate tokens") {
		const concurrency::CancelToken first;
		const concurrency::CancelToken second;

		WHEN("one is cancelled") {
			first.cancel();

			THEN("the other is not") {
				REQUIRE_FALSE(second.isCancelled());
			}
		}
	}
}
