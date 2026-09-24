#include "Components.hpp"
#include <iomanip>
#include <magic_enum/magic_enum.hpp>
#include "../../scene/WorldInfo.hpp"

namespace ecs::component {
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

	std::ostream& Transform::print(std::ostream& os) const {
		os << std::fixed << std::setprecision(2) << std::boolalpha;
		os << "\tPosition: (X:" << position.x << ", Y:" << position.y << ", Z:" << position.z << ")\n";
		os << "\tRotation: (X:" << rotation.x << ", Y:" << rotation.y << ", Z:" << rotation.z << ", W:" << rotation.w <<
				")\n";
		os << "\tScale: (X:" << scale.x << ", Y:" << scale.y << ", Z:" << scale.z << ")\n";
		return os;
	}

	std::ostream& Transform2D::print(std::ostream& os) const {
		os << std::fixed << std::setprecision(2) << std::boolalpha;
		os << "\tPosition: (X:" << position.x << ", Y:" << position.y << ")\n";
		os << "\tScale: (X:" << scale.x << ", Y:" << scale.y << ")\n";
		return os;
	}

	std::ostream& Velocity::print(std::ostream& os) const {
		os << std::fixed << std::setprecision(2) << std::boolalpha;
		os << "\tVelocity: (X:" << velocity.x << ", Y:" << velocity.y << ", Z:" << velocity.z << ")\n";
		os << "\tDesired Velocity: (X:" << desiredVelocity.x << ", Y:" << desiredVelocity.y << ", Z:" << desiredVelocity
				.z << ")\n";
		os << "\tMax Speed: " << maxSpeed << "\n";
		os << "\tAcceleration: " << acceleration << "\n";
		os << "\tDeceleration: " << deceleration << "\n";
		os << "\tCan Move: " << (canMove ? "true" : "false") << "\n";
		return os;
	}

	std::ostream& Camera::print(std::ostream& os) const {
		os << std::fixed << std::setprecision(2) << std::boolalpha;
		os << "\tFOV: " << fov << "\n";
		return os;
	}

	std::ostream& Mesh::print(std::ostream& os) const {
		os << std::fixed << std::setprecision(2) << std::boolalpha;
		os << "\tMesh Handle: " << mesh.id << "\n";
		os << "\tPipeline Type: " << magic_enum::enum_name(pipelineType) << "\n";
		return os;
	}

	std::ostream& Texture::print(std::ostream& os) const {
		os << std::fixed << std::setprecision(2) << std::boolalpha;
		os << "\tTexture Handle: " << texture.id << "\n";
		return os;
	}

	std::ostream& Text::print(std::ostream& os) const {
		os << std::fixed << std::setprecision(2) << std::boolalpha;
		os << "\tText: " << text << "\n";
		os << "\tHorizontal Alignment: " << magic_enum::enum_name(horizontalAlignment) << "\n";
		os << "\tVertical Alignment: " << magic_enum::enum_name(verticalAlignment) << "\n";
		os << "\tAligned: " << (aligned ? "true" : "false") << "\n";
		return os;
	}

	std::ostream& Input::print(std::ostream& os) const {
		os << std::fixed << std::setprecision(2) << std::boolalpha;
		os << "\tMouse Sensitivity: " << mouseSensitivity << "\n";
		os << "\tCommand: \n";
		os << "\t\tMove Forward: " << command.moveForward << "\n";
		os << "\t\tMove Right: " << command.moveRight << "\n";
		os << "\t\tMove Up: " << command.moveUp << "\n";
		os << "\t\tLook Up: " << command.lookUp << "\n";
		os << "\t\tLook Right: " << command.lookRight << "\n";
		os << "\t\tStarted Events: " << command.startedEvents << "\n";
		os << "\t\tRepeated Events: " << command.repeatedEvents << "\n";
		os << "\t\tReleased Events: " << command.releasedEvents << "\n";
		os << "\t\tActive Events: " << command.activeEvents << "\n";
		return os;
	}

	std::ostream& Color::print(std::ostream& os) const {
		os << std::fixed << std::setprecision(2) << std::boolalpha;
		os << "\tColor: (R:" << color.r << ", G:" << color.g << ", B:" << color.b << ", A:" << color.a << ")\n";
		return os;
	}

	std::ostream& Animation::print(std::ostream& os) const {
		os << std::fixed << std::setprecision(2) << std::boolalpha;
		os << "\tType: " << magic_enum::enum_name(type) << "\n";
		os << "\tSpeed: " << speed << "\n";
		os << "\tIntensity: " << intensity << "\n";
		return os;
	}
}
