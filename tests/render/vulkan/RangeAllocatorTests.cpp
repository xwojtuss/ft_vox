#include <catch2/catch_test_macros.hpp>

#include <vector>

#include "render/vulkan/RangeAllocator.hpp"

using render::vulkan::Range;
using render::vulkan::RangeAllocator;

namespace {
	Range grant(RangeAllocator& allocator, const VkDeviceSize size, const VkDeviceSize alignment) {
		return allocator.allocate(size, alignment).value_or(Range{});
	}

	bool wasGranted(const Range& range) {
		return range.allocation != nullptr;
	}

	std::vector<Range> fill(RangeAllocator& allocator, const int count, const VkDeviceSize size) {
		std::vector<Range> ranges(static_cast<size_t>(count));
		for (Range& range: ranges)
			range = grant(allocator, size, 1);
		return ranges;
	}
}

SCENARIO("Meshes get their own range of a shared buffer", "[render][range-allocator]") {
	GIVEN("an empty 1 KiB buffer") {
		RangeAllocator allocator(1024);

		WHEN("two ranges of 100 bytes are requested") {
			const Range first  = grant(allocator, 100, 4);
			const Range second = grant(allocator, 100, 4);

			THEN("both are granted and do not overlap") {
				REQUIRE(wasGranted(first));
				REQUIRE(wasGranted(second));
				const bool firstBeforeSecond = first.offset + 100 <= second.offset;
				const bool secondBeforeFirst = second.offset + 100 <= first.offset;
				REQUIRE((firstBeforeSecond || secondBeforeFirst));
			}
			AND_THEN("the statistics count them") {
				const auto statistics = allocator.statistics();
				REQUIRE(statistics.allocationCount == 2);
				REQUIRE(statistics.allocatedBytes == 200);
				REQUIRE(statistics.freeBytes == 824);
			}
		}

		WHEN("a range is requested with a 32 byte alignment") {
			const Range unaligned = grant(allocator, 5, 4);
			const Range aligned   = grant(allocator, 64, 32);

			THEN("its offset is a multiple of 32") {
				REQUIRE(wasGranted(unaligned));
				REQUIRE(wasGranted(aligned));
				REQUIRE(aligned.offset % 32 == 0);
			}
		}

		WHEN("more than the whole buffer is requested") {
			const auto range = allocator.allocate(2048, 4);

			THEN("nothing is granted") {
				REQUIRE_FALSE(range.has_value());
			}
		}
	}
}

SCENARIO("A freed range is reused and the buffer does not fragment", "[render][range-allocator]") {
	GIVEN("a buffer completely filled with ten 100 byte ranges") {
		RangeAllocator allocator(1000);
		std::vector    ranges = fill(allocator, 10, 100);

		REQUIRE_FALSE(allocator.allocate(1, 1).has_value());

		WHEN("one range is freed") {
			allocator.free(ranges[4].allocation);

			THEN("a new range of the same size fits into the gap") {
				const Range reused = grant(allocator, 100, 1);
				REQUIRE(wasGranted(reused));
				REQUIRE(reused.offset == ranges[4].offset);
			}
		}

		WHEN("two neighbouring ranges are freed") {
			allocator.free(ranges[2].allocation);
			allocator.free(ranges[3].allocation);

			THEN("they merge into one gap that fits a double-sized range") {
				REQUIRE(wasGranted(grant(allocator, 200, 1)));
			}
		}

		WHEN("every range is freed and the buffer is refilled many times") {
			for (int round = 0; round < 50; ++round) {
				for (const Range& range: ranges)
					allocator.free(range.allocation);
				ranges = fill(allocator, 10, 100);
			}

			THEN("it is still a single full buffer with no leaked or lost space") {
				const auto statistics = allocator.statistics();
				REQUIRE(statistics.allocationCount == 10);
				REQUIRE(statistics.freeBytes == 0);
			}
		}

		WHEN("everything is freed") {
			for (const Range& range: ranges)
				allocator.free(range.allocation);

			THEN("the whole buffer is one free range again") {
				const auto statistics = allocator.statistics();
				REQUIRE(statistics.allocationCount == 0);
				REQUIRE(statistics.largestFreeRange == 1000);
				REQUIRE(statistics.freeRangeCount == 1);
			}
		}
	}
}
