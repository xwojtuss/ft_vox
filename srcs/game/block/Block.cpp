#include "Block.hpp"

using namespace game;

Block::Block() : id(0) {
}

Block::Block(const BlockId id) : id(id) {
}

bool Block::hasEntity() const {
	return entity != ecs::nullEntity;
}
