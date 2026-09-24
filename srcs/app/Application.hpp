#pragma once

#include <memory>

#include "../platform/window/IWindow.hpp"
#include "../render/IRenderer.hpp"
#include "../render/gui/IGui.hpp"
#include "../assets/IModelLoader.hpp"
#include "../assets/ITextureLoader.hpp"
#include "../platform/window/glfw/GLFWWindow.hpp"
#include "../render/vulkan/VulkanRenderer.hpp"
#include "../render/gui/vulkan/ImGuiGui.hpp"
#include "../assets/TinyObjLoader.hpp"
#include "../assets/StbTextureLoader.hpp"
#include "../ecs/entity/EntityHandle.hpp"
#include "../ecs/system/types/CameraSystem.hpp"
#include "../ecs/system/types/MovementSystem.hpp"
#include "../ecs/system/types/PlayerInputSystem.hpp"
#include "../ecs/system/types/RenderSystem.hpp"
#include "../ecs/system/types/TextRenderSystem.hpp"
#include "../ecs/system/types/WindowControlSystem.hpp"
#include "../ecs/system/types/ToggleShaderSystem.hpp"
#include "../ecs/system/types/FpsCounter.hpp"
#include "../ecs/system/types/SimpleAnimationSystem.hpp"
#include "../ecs/system/types/GuiSystem.hpp"
#include "../ecs/system/types/ChunkSystem.hpp"
#include "../ecs/component/Components.hpp"
#include "../ecs/World.hpp"

namespace app {
class Application {
private:
	std::unique_ptr<platform::window::IWindow>		m_window;
	std::unique_ptr<render::IRenderer>				m_renderer;
	std::unique_ptr<render::gui::IGui>				m_gui;
	std::unique_ptr<assets::IModelLoader>			m_modelLoader;
	std::unique_ptr<assets::ITextureLoader>			m_textureLoader;
	std::unique_ptr<ecs::World>						m_world;

	void	init();

	/**
	 * Runs once per render frame
	 */
	void	update();

	/**
	 * Runs once per app::simulationFPS
	 */
	void	simulate();
	void	render();

public:
	Application();

	void	run();
};
}