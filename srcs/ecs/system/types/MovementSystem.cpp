#include "MovementSystem.hpp"
#include "../../../scene/WorldInfo.hpp"
#include <string>
#include "../../component/Components.hpp"
#include "../../World.hpp"

using namespace ecs;

MovementSystem::MovementSystem() : ASystem(Dependencies()) {
	m_dependencies.addDependency<component::Transform>();
	m_dependencies.addDependency<component::Velocity>();
	m_dependencies.addDependency<component::Input>();
}

void MovementSystem::onSimulate(const SimulateEvent& event) const {
	for (const Entity& entity: m_entities) {
		component::Transform* transform = m_world->getComponentManager<component::Transform>().getComponent(entity);
		component::Velocity*  velocity  = m_world->getComponentManager<component::Velocity>().getComponent(entity);

		if (!transform || !velocity || !velocity->canMove)
			continue;

		const float speed = glm::length(velocity->velocity);

		if (glm::length(velocity->desiredVelocity) > 0.0f) {
			const glm::vec3 direction = glm::normalize(velocity->desiredVelocity);

			velocity->velocity += direction * velocity->acceleration * event.deltaTime;

			if (glm::length(velocity->velocity) > velocity->maxSpeed)
				velocity->velocity = glm::normalize(velocity->velocity) * velocity->maxSpeed;
		} else if (speed > 0.0f) {
			const float slowedSpeed = std::max(0.0f, speed - velocity->deceleration * event.deltaTime);
			velocity->velocity      = (slowedSpeed > 0.0f)
									? glm::normalize(velocity->velocity) * slowedSpeed
									: glm::vec3(0.0f);
		} else {
			continue;
		}
		const glm::vec3 previousPosition = transform->position;
		transform->position              += velocity->velocity * event.deltaTime;

		m_world->getSystemManager().getDispatcher().emit(PlayerMoveEvent(previousPosition, transform->position));
	}
}

void MovementSystem::onInput(const InputEvent& event) const {
	if (!hasEntity(event.source))
		return;

	component::Velocity*    velocity  = m_world->getComponentManager<component::Velocity>().getComponent(event.source);
	component::Transform*   transform = m_world->getComponentManager<component::Transform>().getComponent(event.source);
	const component::Input* input     = m_world->getComponentManager<component::Input>().getComponent(event.source);

	if (!velocity || !transform || !input)
		return;

	velocity->desiredVelocity = transform->forward() * event.command.moveForward + transform->right() * event.command.
								moveRight + glm::vec3(0, 1, 0) * event.command.moveUp;
	if (transform->canRotate) {
		const float angleX = event.command.lookUp * input->mouseSensitivity;
		const float angleY = event.command.lookRight * input->mouseSensitivity;

		const float pitch        = std::asin(glm::clamp(transform->forward().y, -1.0f, 1.0f));
		float       clampedDelta = glm::clamp(pitch + angleX, -event.command.maxPitch, event.command.maxPitch) - pitch;

		if (clampedDelta > event.command.maxPitch)
			clampedDelta = event.command.maxPitch;
		else if (clampedDelta < -event.command.maxPitch)
			clampedDelta = -event.command.maxPitch;

		const glm::quat rotX = glm::angleAxis(clampedDelta, transform->right());
		const glm::quat rotY = glm::angleAxis(angleY, scene::worldinfo::up);
		transform->rotation  = glm::normalize(rotY * rotX * transform->rotation);
	}
}

void MovementSystem::bindEvents(Dispatcher& dispatcher) {
	dispatcher.subscribe(this, &MovementSystem::onSimulate);
	dispatcher.subscribe(this, &MovementSystem::onInput);
}
