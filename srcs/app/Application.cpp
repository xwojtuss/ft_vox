#include "Application.hpp"
#include "../ecs/system/types/RenderSystem.hpp"
#include "ApplicationInfo.hpp"
#include "../assets/StbTextureLoader.hpp"
#include "../assets/TinyObjLoader.hpp"
#include "../game/block/BlockData.hpp"
#include "../game/world/ChunkManager.hpp"
#include "../ecs/entity/EntityHandle.hpp"
#include "../ecs/system/types/CameraSystem.hpp"
#include "../ecs/system/types/ChunkSystem.hpp"
#include "../ecs/system/types/GuiSystem.hpp"
#include "../ecs/system/types/MovementSystem.hpp"
#include "../ecs/system/types/PlayerInputSystem.hpp"
#include "../ecs/system/types/WindowControlSystem.hpp"
#include "../platform/window/glfw/GLFWWindow.hpp"
#include "../render/gui/vulkan/ImGuiGui.hpp"
#include "../render/vulkan/VulkanRenderer.hpp"

using namespace app;
using namespace ecs;

Application::Application() {
	m_window            = std::make_unique<platform::window::glfw::GLFWWindow>();
	auto vulkanRenderer = std::make_unique<render::vulkan::VulkanRenderer>(*m_window);
	m_gui               = std::make_unique<render::gui::vulkan::ImGuiGui>(vulkanRenderer->getContext(),
															vulkanRenderer->getSwapchain(), *m_window);
	m_renderer      = std::move(vulkanRenderer);
	m_modelLoader   = std::make_unique<assets::TinyObjLoader>();
	m_textureLoader = std::make_unique<assets::StbTextureLoader>();

	auto defaultTextureData         = m_textureLoader->toTextureData("textures/default.png");
	defaultTextureData.pixelPerfect = true;
	const game::block::BlockDatas blockDatas(m_modelLoader->toMeshData("models/cube.obj"), defaultTextureData);
	m_world = std::make_unique<ecs::World>(blockDatas);

	m_window->getInputManager().getKeyInputProcessor().resetBindings();

	init();
}

void Application::init() const {
	m_world->createSystem<CameraSystem>();
	m_world->createSystem<MovementSystem>();
	m_world->createSystem<RenderSystem>();
	m_world->createSystem<WindowControlSystem>(*m_window, *m_gui);
	m_world->createSystem<PlayerInputSystem>(m_window->getInputManager());
	m_world->createSystem<GuiSystem>(*m_gui);
	m_world->createSystem<ChunkSystem>(*m_world, *m_renderer);
	m_world->getSystemManager().onWorldReady();

	EntityHandle         camera = m_world->createEntity();
	component::Transform transform{};
	component::Velocity  velocity{};
	component::Camera    cameraComponent{};
	component::Input     inputComponent{};

	transform.position              = glm::vec3(0.0f, 0.0f, 0.0f);
	transform.rotation              = glm::quat(glm::vec3(0.0f, 0.0f, 0.0f));
	transform.scale                 = glm::vec3(1.0f, 1.0f, 1.0f);
	velocity.acceleration           = 4.5f;
	velocity.deceleration           = 10.0f;
	velocity.maxSpeed               = 10.0f;
	velocity.velocity               = glm::vec3(0.0f);
	velocity.desiredVelocity        = glm::vec3(0.0f);
	cameraComponent.fov             = 90.0f;
	inputComponent.mouseSensitivity = 0.002f;
	camera.addComponent(transform);
	camera.addComponent(velocity);
	camera.addComponent(cameraComponent);
	camera.addComponent(inputComponent);

	camera.registerToSystem<MovementSystem>();
	camera.registerToSystem<PlayerInputSystem>();
	camera.registerToSystem<CameraSystem>();
	camera.registerToSystem<WindowControlSystem>();

	m_renderer->setClearColor(0x0a2882);
}

void Application::run() {
	while (!m_window->shouldClose()) {
		m_window->pollEvents();
		simulate();
		update();
		render();
	}
}

void Application::update() const {
	m_world->getSystemManager().onRender(m_window->getAspectRatio(), m_window->getTime());
}

void Application::simulate() {
	const double time = m_window->getTime();
	const double dt   = time - m_lastSimulateTime;

	if (m_window->wasResized() || dt < simulationFrameRate) {
		return;
	}
	m_lastSimulateTime = time;

	m_world->getSystemManager().onSimulate(static_cast<float>(dt), static_cast<float>(time));
}

void Application::render() const {
	m_renderer->render(m_world->getSystemManager());
}
