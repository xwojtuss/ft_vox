#pragma once

#include "../../ecs/entity/Entity.hpp"

namespace game {
	using BlockId = unsigned int;

	struct Block {
		BlockId     id;
		ecs::Entity entity = ecs::nullEntity;

		Block();
		explicit Block(BlockId id);

		[[nodiscard]] bool hasEntity() const;
	};
}
