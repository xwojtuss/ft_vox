#include "profiling/FrameTimes.hpp"

#include <algorithm>

using namespace profiling;

FrameTimes::FrameTimes(const double windowSeconds) : m_windowSeconds(windowSeconds) {
}

void FrameTimes::record(const double timestamp, const float frameSeconds) {
	m_samples.push_back({.timestamp = timestamp, .seconds = frameSeconds});
	while (timestamp - m_samples.front().timestamp > m_windowSeconds)
		m_samples.pop_front();
}

float FrameTimes::average() const {
	if (m_samples.empty())
		return 0.0f;

	float total = 0.0f;
	for (const Sample& sample: m_samples)
		total += sample.seconds;
	return total / static_cast<float>(m_samples.size());
}

float FrameTimes::worst() const {
	float worst = 0.0f;
	for (const Sample& sample: m_samples)
		worst = std::max(worst, sample.seconds);
	return worst;
}

size_t FrameTimes::size() const {
	return m_samples.size();
}

std::vector<float> FrameTimes::values() const {
	std::vector<float> values;
	values.reserve(m_samples.size());
	for (const Sample& sample: m_samples)
		values.push_back(sample.seconds);
	return values;
}
