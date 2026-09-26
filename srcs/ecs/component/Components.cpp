#include "Components.hpp"

#include <tuple>
#include <magic_enum/magic_enum.hpp>

#include "../../scene/WorldInfo.hpp"

using namespace ecs::component;

glm::vec3 Transform::forward() const {
	return rotation * scene::worldinfo::forward;
}

glm::vec3 Transform::right() const {
	return rotation * scene::worldinfo::right;
}

glm::vec3 Transform::left() const {
	return rotation * scene::worldinfo::left;
}

glm::vec3 Transform::up() const {
	return rotation * scene::worldinfo::up;
}

glm::vec3 Transform::down() const {
	return rotation * scene::worldinfo::down;
}

glm::mat4 Transform::toModelMatrix() const {
	const glm::mat4 translation = glm::translate(glm::mat4(1.0f), position);
	const glm::mat4 rotationMat = glm::toMat4(rotation);
	const glm::mat4 scaleMat    = glm::scale(glm::mat4(1.0f), scale);

	return translation * rotationMat * scaleMat;
}

std::format_context::iterator std::formatter<Transform>::format(const Transform&     transform,
																std::format_context& context) const {
	const auto& [position, rotation, scale] = std::tie(transform.position, transform.rotation, transform.scale);
	return std::format_to(context.out(),
						"\tPosition: (X:{:.2f}, Y:{:.2f}, Z:{:.2f})\n"
						"\tRotation: (X:{:.2f}, Y:{:.2f}, Z:{:.2f}, W:{:.2f})\n"
						"\tScale: (X:{:.2f}, Y:{:.2f}, Z:{:.2f})\n",
						position.x, position.y, position.z,
						rotation.x, rotation.y, rotation.z, rotation.w,
						scale.x, scale.y, scale.z);
}

std::format_context::iterator std::formatter<Velocity>::format(const Velocity&       velocity,
																std::format_context& context) const {
	return std::format_to(context.out(),
						"\tVelocity: (X:{:.2f}, Y:{:.2f}, Z:{:.2f})\n"
						"\tDesired Velocity: (X:{:.2f}, Y:{:.2f}, Z:{:.2f})\n"
						"\tMax Speed: {:.2f}\n"
						"\tAcceleration: {:.2f}\n"
						"\tDeceleration: {:.2f}\n"
						"\tCan Move: {}\n",
						velocity.velocity.x, velocity.velocity.y, velocity.velocity.z,
						velocity.desiredVelocity.x, velocity.desiredVelocity.y, velocity.desiredVelocity.z,
						velocity.maxSpeed, velocity.acceleration, velocity.deceleration, velocity.canMove);
}

std::format_context::iterator std::formatter<Camera>::format(const Camera& camera, std::format_context& context) const {
	return std::format_to(context.out(), "\tFOV: {:.2f}\n", camera.fov);
}

std::format_context::iterator std::formatter<Mesh>::format(const Mesh& mesh, std::format_context& context) const {
	return std::format_to(context.out(), "\tMesh Handle: {}\n\tPipeline Type: {}\n", mesh.mesh.id,
						magic_enum::enum_name(mesh.pipelineType));
}

std::format_context::iterator std::formatter<Texture>::format(const Texture&     texture,
															std::format_context& context) const {
	return std::format_to(context.out(), "\tTexture Handle: {}\n", texture.texture.id);
}

std::format_context::iterator std::formatter<Input>::format(const Input& input, std::format_context& context) const {
	const render::input::InputCommand& command = input.command;
	return std::format_to(context.out(),
						"\tMouse Sensitivity: {:.2f}\n"
						"\tCommand: \n"
						"\t\tMove Forward: {:.2f}\n"
						"\t\tMove Right: {:.2f}\n"
						"\t\tMove Up: {:.2f}\n"
						"\t\tLook Up: {:.2f}\n"
						"\t\tLook Right: {:.2f}\n"
						"\t\tStarted Events: {}\n"
						"\t\tRepeated Events: {}\n"
						"\t\tReleased Events: {}\n"
						"\t\tActive Events: {}\n",
						input.mouseSensitivity, command.moveForward, command.moveRight, command.moveUp,
						command.lookUp, command.lookRight, command.startedEvents, command.repeatedEvents,
						command.releasedEvents, command.activeEvents);
}
