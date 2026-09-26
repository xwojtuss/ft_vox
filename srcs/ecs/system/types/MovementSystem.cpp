#include "MovementSystem.hpp"

#include <algorithm>
#include <vector>

#include "../../../scene/WorldInfo.hpp"

using namespace ecs;

void MovementSystem::onSimulate(const SimulateEvent& event) const {
	std::vector<PlayerMoveEvent> moves;

	for (auto&& [entity, transform, velocity, input]: entities()) {
		if (!velocity.canMove)
			continue;

		const float speed = glm::length(velocity.velocity);

		if (glm::length(velocity.desiredVelocity) > 0.0f) {
			const glm::vec3 direction = glm::normalize(velocity.desiredVelocity);

			velocity.velocity += direction * velocity.acceleration * event.deltaTime;

			if (glm::length(velocity.velocity) > velocity.maxSpeed)
				velocity.velocity = glm::normalize(velocity.velocity) * velocity.maxSpeed;
		} else if (speed > 0.0f) {
			const float slowedSpeed = std::max(0.0f, speed - velocity.deceleration * event.deltaTime);
			velocity.velocity       = (slowedSpeed > 0.0f)
									? glm::normalize(velocity.velocity) * slowedSpeed
									: glm::vec3(0.0f);
		} else {
			continue;
		}
		const glm::vec3 previousPosition = transform.position;
		transform.position               += velocity.velocity * event.deltaTime;
		moves.emplace_back(previousPosition, transform.position);
	}

	for (const PlayerMoveEvent& move: moves)
		m_world->getSystemManager().getDispatcher().emit(move);
}

void MovementSystem::onInput(const InputEvent& event) const {
	if (!processes(event.source))
		return;

	EntityHandle                       player    = m_world->getEntity(event.source);
	auto&                              velocity  = player.get<component::Velocity>();
	auto&                              transform = player.get<component::Transform>();
	const component::Input&            input     = player.get<component::Input>();
	const render::input::InputCommand& command   = event.command;

	velocity.desiredVelocity = transform.forward() * command.moveForward + transform.right() * command.moveRight
								+ scene::worldinfo::up * command.moveUp;
	if (!transform.canRotate)
		return;

	const float angleX = command.lookUp * input.mouseSensitivity;
	const float angleY = command.lookRight * input.mouseSensitivity;

	const float pitch        = std::asin(glm::clamp(transform.forward().y, -1.0f, 1.0f));
	const float pitchDelta   = glm::clamp(pitch + angleX, -command.maxPitch, command.maxPitch) - pitch;
	const float clampedDelta = glm::clamp(pitchDelta, -command.maxPitch, command.maxPitch);

	const glm::quat rotX = glm::angleAxis(clampedDelta, transform.right());
	const glm::quat rotY = glm::angleAxis(angleY, scene::worldinfo::up);
	transform.rotation   = glm::normalize(rotY * rotX * transform.rotation);
}

void MovementSystem::bindEvents(Dispatcher& dispatcher) {
	dispatcher.subscribe(this, &MovementSystem::onSimulate);
	dispatcher.subscribe(this, &MovementSystem::onInput);
}
