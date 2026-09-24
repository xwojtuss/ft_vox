#pragma once

#include <vector>

namespace game::world {
	class Perlin2DMap {
	private:
		std::vector<float> m_data;
		unsigned int       m_width;
		unsigned int       m_height;
		unsigned int       m_octaves;
		float              m_scale;

		void                generate();
		[[nodiscard]] float sampleValue(int x, int y) const;

		inline static float perlinNoise(float x, float y);

	public:
		Perlin2DMap(unsigned int width, unsigned int height, unsigned int octaves, float scale);

		[[nodiscard]] float getValue(int x, int y) const;
		[[nodiscard]] float getNormalizedValue(int x, int y) const;
	};
}
