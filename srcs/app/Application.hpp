#pragma once

#include <memory>

#include "../platform/window/IWindow.hpp"
#include "../render/IRenderer.hpp"
#include "../render/gui/IGui.hpp"
#include "../assets/IModelLoader.hpp"
#include "../assets/ITextureLoader.hpp"
#include "../ecs/World.hpp"

namespace app {
	class Application {
	private:
		std::unique_ptr<platform::window::IWindow> m_window;
		std::unique_ptr<render::IRenderer>         m_renderer;
		std::unique_ptr<render::gui::IGui>         m_gui;
		std::unique_ptr<assets::IModelLoader>      m_modelLoader;
		std::unique_ptr<assets::ITextureLoader>    m_textureLoader;
		std::unique_ptr<ecs::World>                m_world;
		double                                     m_lastSimulateTime = 0.0;

		void init() const;

		/**
		* Runs once per render frame
		*/
		void update() const;

		/**
		* Runs once per app::simulationFPS
		*/
		void simulate();
		void render() const;

	public:
		Application();

		void run();
	};
}
