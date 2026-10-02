#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "profiling/FrameTimes.hpp"

using Catch::Approx;

SCENARIO("Frame times are summarised over the last few seconds", "[profiling][frame-times]") {
	GIVEN("no recorded frames") {
		const profiling::FrameTimes frameTimes;

		THEN("the average and the worst frame time are zero") {
			REQUIRE(frameTimes.average() == 0.0f);
			REQUIRE(frameTimes.worst() == 0.0f);
			REQUIRE(frameTimes.size() == 0);
		}
	}

	GIVEN("three frames inside the window") {
		profiling::FrameTimes frameTimes(3.0);
		frameTimes.record(1.0, 0.010f);
		frameTimes.record(1.1, 0.020f);
		frameTimes.record(1.2, 0.060f);

		THEN("the average is the mean of the frame times") {
			REQUIRE(frameTimes.average() == Approx(0.030f));
		}
		AND_THEN("the worst is the longest frame") {
			REQUIRE(frameTimes.worst() == Approx(0.060f));
		}
		AND_THEN("the values are listed oldest first") {
			REQUIRE(frameTimes.values() == std::vector<float>{0.010f, 0.020f, 0.060f});
		}

		WHEN("a frame is recorded after the window has passed the first ones") {
			frameTimes.record(4.15, 0.016f);

			THEN("older frames no longer count") {
				REQUIRE(frameTimes.size() == 2);
				REQUIRE(frameTimes.worst() == Approx(0.060f));
			}

			AND_WHEN("the slow frame also leaves the window") {
				frameTimes.record(5.5, 0.016f);

				THEN("the worst value drops with it") {
					REQUIRE(frameTimes.worst() == Approx(0.016f));
					REQUIRE(frameTimes.average() == Approx(0.016f));
				}
			}
		}
	}
}
