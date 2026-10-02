#pragma once

#include <cstddef>
#include <deque>
#include <vector>

namespace profiling {
	constexpr double defaultFrameTimeWindowSeconds = 3.0;

	class FrameTimes {
	private:
		struct Sample {
			double timestamp;
			float  seconds;
		};

		std::deque<Sample> m_samples;
		double             m_windowSeconds;

	public:
		explicit FrameTimes(double windowSeconds = defaultFrameTimeWindowSeconds);

		void record(double timestamp, float frameSeconds);

		[[nodiscard]] float              average() const;
		[[nodiscard]] float              worst() const;
		[[nodiscard]] size_t             size() const;
		[[nodiscard]] std::vector<float> values() const;
	};
}
