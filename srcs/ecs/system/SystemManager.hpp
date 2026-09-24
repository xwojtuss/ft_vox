#pragma once

#include <unordered_map>
#include <memory>
#include <typeindex>

#include "ASystem.hpp"
#include "Dispatcher.hpp"
#include "DispatcherEvents.hpp"

namespace ecs {
	class SystemManager {
	private:
		std::unordered_map<std::type_index, std::unique_ptr<ASystem>> m_systems;
		Dispatcher                                                    m_dispatcher;

	public:
		template<typename SystemType, typename... Args>
		SystemType& addSystem(Args&&... args);

		template<typename SystemType>
		[[nodiscard]] SystemType* getSystem();

		void                      onWorldReady();
		void                      onRender(float aspectRatio, double time);
		void                      onTextDraw(render::IRenderer& renderer);
		void                      onRendererDraw(render::IRenderer& renderer);
		void                      onRendererFrame(render::IRenderer& renderer);
		void                      onSimulate(float deltaTime, float time);
		[[nodiscard]] Dispatcher& getDispatcher();
	};
}

#include "SystemManager.tpp"
