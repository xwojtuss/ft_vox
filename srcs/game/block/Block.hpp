#pragma once

#include "../../ecs/entity/Entity.hpp"

namespace game {
	using BlockId = unsigned int;

	struct Block {
		BlockId     id;
		ecs::Entity entity = -1;

		Block();
		explicit Block(BlockId id);

		[[nodiscard]] bool hasEntity() const;
	};
}
