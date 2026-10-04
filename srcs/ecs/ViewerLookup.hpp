#pragma once

#include <optional>

#include "game/planet/ChunkOrder.hpp"

namespace ecs {
	class Registry;

	[[nodiscard]] std::optional<game::planet::Viewer> findViewer(Registry& registry);
}
