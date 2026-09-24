#pragma once

#include <string>
#include <ostream>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

#include "Component.hpp"
#include "../../assets/Resources.hpp"
#include "../../render/input/InputTypes.hpp"

namespace ecs::component {
	struct Transform : public Component<Transform> {
		glm::vec3 position  = glm::vec3(0.0f, 0.0f, 0.0f);
		glm::quat rotation  = glm::quat(glm::vec3(0.0f, 0.0f, 0.0f));
		glm::vec3 scale     = glm::vec3(1.0f, 1.0f, 1.0f);
		bool      canRotate = true;

		Transform() : Component("Transform") {
		}

		[[nodiscard]] glm::vec3 forward() const;
		[[nodiscard]] glm::vec3 right() const;
		[[nodiscard]] glm::vec3 left() const;
		[[nodiscard]] glm::vec3 up() const;
		[[nodiscard]] glm::vec3 down() const;
		[[nodiscard]] glm::mat4 toModelMatrix() const;

		std::ostream& print(std::ostream& os) const override;
	};

	struct Transform2D : public Component<Transform2D> {
		glm::vec2 position;
		glm::vec2 scale;

		Transform2D() : Component("Transform2D") {
		}

		std::ostream& print(std::ostream& os) const override;
	};

	struct Velocity : public Component<Velocity> {
		glm::vec3 velocity;
		glm::vec3 desiredVelocity;
		float     maxSpeed{};
		float     acceleration{};
		float     deceleration{};
		bool      canMove = true;

		Velocity() : Component("Velocity") {
		}

		std::ostream& print(std::ostream& os) const override;
	};

	struct Camera : public Component<Camera> {
		glm::mat4 view;
		glm::mat4 projection;
		float     fov{};

		Camera() : Component("Camera") {
		}

		std::ostream& print(std::ostream& os) const override;
	};

	struct Mesh : public Component<Mesh> {
		assets::MeshHandle   mesh;
		assets::PipelineType pipelineType = assets::PipelineType::Textured;

		Mesh() : Component("Mesh") {
		}

		std::ostream& print(std::ostream& os) const override;
	};

	struct Texture : public Component<Texture> {
		assets::TextureHandle texture;

		Texture() : Component("Texture") {
		}

		std::ostream& print(std::ostream& os) const override;
	};

	enum HAlignment {
		Left,
		Center,
		Right
	};

	enum VAlignment {
		Top    = Left,
		Middle = Center,
		Bottom = Right
	};

	struct Text : public Component<Text>, public Mesh {
		std::string text;
		HAlignment  horizontalAlignment = Left;
		VAlignment  verticalAlignment   = Top;
		bool        aligned             = false;

		Text() : Component<Text>("Text") {
		}

		explicit Text(assets::MeshHandle textMeshHandle) : Component<Text>("Text") {
			pipelineType = assets::PipelineType::Text;
			mesh         = textMeshHandle;
		}

		std::ostream& print(std::ostream& os) const override;
	};

	struct Input : public Component<Input> {
		render::input::InputCommand command;
		float                       mouseSensitivity{};

		Input() : Component("Input") {
		}

		std::ostream& print(std::ostream& os) const override;
	};

	struct Color : public Component<Color> {
		glm::vec4 color;

		Color() : Component("Color") {
		}

		std::ostream& print(std::ostream& os) const override;
	};

	enum AnimationType {
		Spin,
		Bounce,
		Jitter,
		Random,
		Circle,
		Pulse
	};

	struct Animation : public Component<Animation> {
		float         speed     = 1.0f;
		float         intensity = 1.0f;
		AnimationType type      = AnimationType::Spin;

		Animation() : Component("Animation") {
		}

		std::ostream& print(std::ostream& os) const override;
	};
}
