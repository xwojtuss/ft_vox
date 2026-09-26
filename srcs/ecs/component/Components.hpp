#pragma once

#include <format>
#include <string_view>
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

#include "../../assets/Resources.hpp"
#include "../../render/input/InputTypes.hpp"

namespace ecs::component {
	struct Transform {
		glm::vec3 position  = glm::vec3(0.0f, 0.0f, 0.0f);
		glm::quat rotation  = glm::quat(glm::vec3(0.0f, 0.0f, 0.0f));
		glm::vec3 scale     = glm::vec3(1.0f, 1.0f, 1.0f);
		bool      canRotate = true;

		[[nodiscard]] glm::vec3 forward() const;
		[[nodiscard]] glm::vec3 right() const;
		[[nodiscard]] glm::vec3 left() const;
		[[nodiscard]] glm::vec3 up() const;
		[[nodiscard]] glm::vec3 down() const;
		[[nodiscard]] glm::mat4 toModelMatrix() const;
	};

	struct Velocity {
		glm::vec3 velocity        = glm::vec3(0.0f);
		glm::vec3 desiredVelocity = glm::vec3(0.0f);
		float     maxSpeed{};
		float     acceleration{};
		float     deceleration{};
		bool      canMove = true;
	};

	struct Camera {
		glm::mat4 view       = glm::mat4(1.0f);
		glm::mat4 projection = glm::mat4(1.0f);
		float     fov{};
		float     nearPlane = 0.1f;
		float     farPlane  = 1000.0f;
	};

	struct Mesh {
		assets::MeshHandle   mesh;
		assets::PipelineType pipelineType = assets::PipelineType::Textured;
	};

	struct Texture {
		assets::TextureHandle texture;
	};

	struct Input {
		render::input::InputCommand command;
		float                       mouseSensitivity{};
	};
}

template<>
struct std::formatter<ecs::component::Transform> : std::formatter<std::string_view> {
	std::format_context::iterator format(const ecs::component::Transform& transform,
										std::format_context&              context) const;
};

template<>
struct std::formatter<ecs::component::Velocity> : std::formatter<std::string_view> {
	std::format_context::iterator format(const ecs::component::Velocity& velocity, std::format_context& context) const;
};

template<>
struct std::formatter<ecs::component::Camera> : std::formatter<std::string_view> {
	std::format_context::iterator format(const ecs::component::Camera& camera, std::format_context& context) const;
};

template<>
struct std::formatter<ecs::component::Mesh> : std::formatter<std::string_view> {
	std::format_context::iterator format(const ecs::component::Mesh& mesh, std::format_context& context) const;
};

template<>
struct std::formatter<ecs::component::Texture> : std::formatter<std::string_view> {
	std::format_context::iterator format(const ecs::component::Texture& texture, std::format_context& context) const;
};

template<>
struct std::formatter<ecs::component::Input> : std::formatter<std::string_view> {
	std::format_context::iterator format(const ecs::component::Input& input, std::format_context& context) const;
};
