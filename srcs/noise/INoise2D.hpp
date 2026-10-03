#pragma once

#include <cstdint>
#include <memory>

namespace noise {
	struct FractalSettings {
		float frequency;
		int   octaves;
	};

	class INoise2D {
	public:
		virtual ~INoise2D() = default;

		/** A value between -1 and 1 that depends only on the seed the noise was created with and on the position. */
		[[nodiscard]] virtual float sample(float x, float y) const = 0;
	};

	[[nodiscard]] std::unique_ptr<INoise2D> createFractalPerlin2D(std::uint64_t seed, FractalSettings settings);
}
