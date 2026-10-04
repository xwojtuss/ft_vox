#include "ecs/ViewerLookup.hpp"

#include "ecs/Registry.hpp"
#include "ecs/component/Components.hpp"

std::optional<game::planet::Viewer> ecs::findViewer(Registry& registry) {
	for (auto&& [entity, transform, camera]: registry.query<const component::Transform, const component::Camera>()) {
		static_cast<void>(entity);
		static_cast<void>(camera);
		return game::planet::Viewer{.position = transform.position, .forward = transform.forward()};
	}
	return std::nullopt;
}
