#include "app/Application.hpp"
#include "ecs/component/Components.hpp"
#include "ecs/system/types/RenderSystem.hpp"
#include "app/ApplicationInfo.hpp"
#include "assets/StbTextureLoader.hpp"
#include "assets/TinyObjLoader.hpp"
#include "game/block/BlockData.hpp"
#include "game/planet/ChunkManager.hpp"
#include "ecs/entity/EntityHandle.hpp"
#include "ecs/system/types/CameraSystem.hpp"
#include "ecs/system/types/ChunkSystem.hpp"
#include "ecs/system/types/GuiSystem.hpp"
#include "ecs/system/types/MovementSystem.hpp"
#include "ecs/system/types/PlayerInputSystem.hpp"
#include "ecs/system/types/WindowControlSystem.hpp"
#include "platform/window/glfw/GLFWWindow.hpp"
#include "render/gui/vulkan/ImGuiGui.hpp"
#include "render/vulkan/VulkanRenderer.hpp"

using namespace app;
using namespace ecs;

Application::Application() {
	m_window            = std::make_unique<platform::window::glfw::GLFWWindow>();
	auto vulkanRenderer = std::make_unique<render::vulkan::VulkanRenderer>(*m_window);
	m_gui               = std::make_unique<render::gui::vulkan::ImGuiGui>(vulkanRenderer->getContext(),
																		  vulkanRenderer->getSwapchain(), *m_window);
	m_renderer          = std::move(vulkanRenderer);
	m_modelLoader       = std::make_unique<assets::TinyObjLoader>();
	m_textureLoader     = std::make_unique<assets::StbTextureLoader>();

	auto defaultTextureData         = m_textureLoader->toTextureData("textures/default.png");
	defaultTextureData.pixelPerfect = true;
	game::block::BlockDatas blockDatas(m_modelLoader->toMeshData("models/cube.obj"), std::move(defaultTextureData));
	m_registry = std::make_unique<Registry>(std::move(blockDatas));

	m_window->getInputManager().getKeyInputProcessor().resetBindings();

	init();
}

void Application::init() const {
	m_registry->createSystem<CameraSystem>();
	m_registry->createSystem<MovementSystem>();
	m_registry->createSystem<RenderSystem>();
	m_registry->createSystem<WindowControlSystem>(*m_window, *m_gui);
	m_registry->createSystem<PlayerInputSystem>(m_window->getInputManager());
	m_registry->createSystem<GuiSystem>(*m_gui);
	m_registry->createSystem<ChunkSystem>(*m_registry, *m_renderer);
	m_registry->getSystemManager().onRegistryReady();

	EntityHandle player = m_registry->createEntity();
	player.add(component::Transform{.position = glm::vec3(0.0f, 100.0f, 0.0f)});
	player.add(component::Velocity{.maxSpeed = 10.0f, .acceleration = 4.5f, .deceleration = 10.0f});
	player.add(component::Camera{.fov = 90.0f});
	player.add(component::Input{.mouseSensitivity = 0.002f});

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
	m_registry->getSystemManager().onRender(m_window->getAspectRatio(), m_window->getTime());
}

void Application::simulate() {
	const double time = m_window->getTime();
	const double dt   = time - m_lastSimulateTime;

	if (m_window->wasResized() || dt < simulationFrameRate) {
		return;
	}
	m_lastSimulateTime = time;

	m_registry->getSystemManager().onSimulate(static_cast<float>(dt), static_cast<float>(time));
}

void Application::render() const {
	m_renderer->render(m_registry->getSystemManager());
}
