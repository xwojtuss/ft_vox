#include "Dependencies.hpp"
#include "../component/Component.hpp"

using namespace ecs;

Dependencies::Dependencies() : mask(0) {
}

bool Dependencies::matches(const Dependencies& other) const {
	return (mask & other.mask) == mask;
}
