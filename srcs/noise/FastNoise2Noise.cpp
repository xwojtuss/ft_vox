#include <FastNoise/FastNoise.h>

#include "noise/INoise2D.hpp"

namespace {
	class FastNoise2Fractal final : public noise::INoise2D {
	private:
		FastNoise::SmartNode<FastNoise::FractalFBm> m_generator;
		float                                       m_frequency;
		int                                         m_seed;

	public:
		FastNoise2Fractal(const std::uint64_t seed, const noise::FractalSettings settings) :
			m_generator(FastNoise::New<FastNoise::FractalFBm>()), m_frequency(settings.frequency),
			m_seed(static_cast<int>(seed ^ (seed >> 32U))) {
			m_generator->SetSource(FastNoise::New<FastNoise::Perlin>());
			m_generator->SetOctaveCount(settings.octaves);
		}

		[[nodiscard]] float sample(const float x, const float y) const override {
			return m_generator->GenSingle2D(x * m_frequency, y * m_frequency, m_seed);
		}
	};
}

std::unique_ptr<noise::INoise2D> noise::createFractalPerlin2D(const std::uint64_t   seed,
															  const FractalSettings settings) {
	return std::make_unique<FastNoise2Fractal>(seed, settings);
}
