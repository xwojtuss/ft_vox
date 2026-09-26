#pragma once

#include <cstdint> // for std::uint32_t
#include <limits>

namespace ecs {
	enum class Entity : std::uint32_t {
	};

	constexpr auto nullEntity = Entity{std::numeric_limits<std::uint32_t>::max()};
}
