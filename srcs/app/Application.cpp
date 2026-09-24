#include "Application.hpp"

// TEMPORARY
#include "../game/block/BlockData.hpp"
#include "../game/world/Chunk.hpp"
#include "../game/world/ChunkMesher.hpp"
#include "../game/world/ChunkManager.hpp"

using namespace app;

Application::Application() {
	m_window = std::make_unique<platform::window::glfw::GLFWWindow>();
	m_renderer = std::make_unique<render::vulkan::VulkanRenderer >(*m_window);
	auto* vulkanRenderer = static_cast<render::vulkan::VulkanRenderer*>(m_renderer.get());
	m_gui = std::make_unique<render::gui::vulkan::ImGuiGui>(vulkanRenderer->getContext(), vulkanRenderer->getSwapchain(), *m_window);
	m_modelLoader = std::make_unique<assets::TinyObjLoader>();
	m_textureLoader = std::make_unique<assets::StbTextureLoader>();

	auto defaultTextureData = m_textureLoader->toTextureData("textures/default.png");
	defaultTextureData.pixelPerfect = true;
	game::block::BlockDatas blockDatas(m_modelLoader->toMeshData("models/cube.obj"), defaultTextureData);
	m_world = std::make_unique<ecs::World>(blockDatas);

	m_window->getInputManager().getKeyInputProcessor().resetBindings();

	init();
}

void	Application::init() {
	m_world->createSystem<ecs::CameraSystem>();
	m_world->createSystem<ecs::MovementSystem>();
	m_world->createSystem<ecs::RenderSystem>();
	m_world->createSystem<ecs::TextRenderSystem>();
	m_world->createSystem<ecs::WindowControlSystem>(*m_window, *m_gui);
	m_world->createSystem<ecs::PlayerInputSystem>(m_window->getInputManager());
	m_world->createSystem<ecs::FpsCounter>();
	m_world->createSystem<ecs::ToggleShaderSystem>();
	m_world->createSystem<ecs::SimpleAnimationSystem>();
	m_world->createSystem<ecs::GuiSystem>(*m_gui);
	m_world->createSystem<ecs::ChunkSystem>(*m_world, *m_renderer);
	m_world->getSystemManager().onWorldReady();

	ecs::EntityHandle camera = m_world->createEntity();
	ecs::component::Transform transform{};
	ecs::component::Velocity velocity{};
	ecs::component::Camera cameraComponent{};
	ecs::component::Input inputComponent{};

	transform.position = glm::vec3(0.0f, 0.0f, 0.0f);
	transform.rotation = glm::quat(glm::vec3(0.0f, 0.0f, 0.0f));
	transform.scale = glm::vec3(1.0f, 1.0f, 1.0f);
	velocity.acceleration = 4.5f;
	velocity.decelleration = 10.0f;
	velocity.maxSpeed = 10.0f;
	velocity.velocity = glm::vec3(0.0f);
	velocity.desiredVelocity = glm::vec3(0.0f);
	cameraComponent.fov = 90.0f;
	inputComponent.mouseSensitivity = 0.002f;
	camera.addComponent(transform);
	camera.addComponent(velocity);
	camera.addComponent(cameraComponent);
	camera.addComponent(inputComponent);

	camera.registerToSystem<ecs::MovementSystem>();
	camera.registerToSystem<ecs::PlayerInputSystem>();
	camera.registerToSystem<ecs::CameraSystem>();
	camera.registerToSystem<ecs::WindowControlSystem>();
	camera.registerToSystem<ecs::ToggleShaderSystem>();

	ecs::EntityHandle floor = m_world->createEntity();
	ecs::component::Transform floorTransform{};
	ecs::component::Mesh floorMesh{};
	ecs::component::Texture floorTexture{};
	float floorSize = 100.0f;
	floorTransform.position = glm::vec3(floorSize / 2.0f, -5.0f, -floorSize / 2.0f);
	floorTransform.scale = glm::vec3(floorSize, floorSize, 1.0f);
	floorTransform.rotation = glm::quat(glm::vec3(0.0f, 0.0f, 0.0f));
	floorTransform.rotation = glm::rotate(floorTransform.rotation, glm::radians(180.0f), scene::worldinfo::up);
	floorTransform.rotation = glm::rotate(floorTransform.rotation, glm::radians(90.0f), scene::worldinfo::left);
	assets::TextureData floorTextureData = m_textureLoader->toTextureData("textures/medival.jpg");
	assets::MeshData floorMeshData;
	floorMeshData.vertices = {
		{{0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f}},
		{{1.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 0.0f}},
		{{1.0f, 1.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}},
		{{0.0f, 1.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}},
	};
	floorMeshData.indices = {
		0, 1, 2,
		2, 3, 0
	};
	floorMeshData.scaleTextureCoordinates(floorSize / 10.0f);
	floorMesh.mesh = m_renderer->createMesh(floorMeshData);
	floorTexture.texture = m_renderer->createTexture(floorTextureData);
	floorMesh.pipelineType = assets::PipelineType::Textured;
	floor.addComponent(floorTransform);
	floor.addComponent(floorMesh);
	floor.addComponent(floorTexture);
	floor.registerToSystem<ecs::RenderSystem>();

	m_renderer->setClearColor(0x0a2882);
}

void	Application::run() {
	while (!m_window->shouldClose()) {
		m_window->pollEvents();
		simulate();
		update();
		render();
	}
}

void	Application::update() {
	m_world->getSystemManager().onRender(m_window->getAspectRatio(), m_window->getTime());
}

void	Application::simulate() {
	double time = m_window->getTime();
	static double lastSimulateTime = 0.0;
	auto dt = time - lastSimulateTime;
	
	if (m_window->wasResized() || time - lastSimulateTime < simulationFrameRate) {
		return;
	}
	lastSimulateTime = time;

	m_world->getSystemManager().onSimulate(static_cast<float>(dt), static_cast<float>(time));
}

void	Application::render() {
	m_renderer->render(m_world->getSystemManager());
}
