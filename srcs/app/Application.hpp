#pragma once

#include <memory>

#include "concurrency/IThreadPool.hpp"
#include "platform/window/IWindow.hpp"
#include "render/IRenderer.hpp"
#include "render/gui/IGui.hpp"
#include "assets/IModelLoader.hpp"
#include "assets/ITextureLoader.hpp"
#include "ecs/Registry.hpp"
#include "profiling/ClientStats.hpp"
#include "profiling/ServerStats.hpp"

namespace app {
	class Application {
	private:
		profiling::ClientStats                     m_clientStats;
		profiling::ServerStats                     m_serverStats;
		std::unique_ptr<concurrency::IThreadPool>  m_threadPool;
		std::unique_ptr<platform::window::IWindow> m_window;
		std::unique_ptr<render::IRenderer>         m_renderer;
		std::unique_ptr<render::gui::IGui>         m_gui;
		std::unique_ptr<assets::IModelLoader>      m_modelLoader;
		std::unique_ptr<assets::ITextureLoader>    m_textureLoader;
		std::unique_ptr<ecs::Registry>             m_registry;
		double                                     m_lastSimulateTime = 0.0;
		double                                     m_lastFrameTime    = 0.0;

		void init();

		/**
		 * Runs once per render frame
		 */
		void update() const;

		/**
		 * Runs once per app::simulationFPS
		 */
		void simulate();
		void render() const;
		void recordFrameTime();

	public:
		Application();

		void run();
	};
}
