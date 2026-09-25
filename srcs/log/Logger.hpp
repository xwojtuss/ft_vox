#pragma once

#include <memory>
#include <utility>
#include <spdlog/logger.h>

namespace logging {
	class Logger {
	private:
		std::shared_ptr<spdlog::logger> m_logger;

	public:
		explicit Logger(std::shared_ptr<spdlog::logger> logger);

		template<typename... Args>
		void trace(spdlog::format_string_t<Args...> format, Args&&... args) const;

		template<typename... Args>
		void debug(spdlog::format_string_t<Args...> format, Args&&... args) const;

		template<typename... Args>
		void info(spdlog::format_string_t<Args...> format, Args&&... args) const;

		template<typename... Args>
		void warn(spdlog::format_string_t<Args...> format, Args&&... args) const;

		template<typename... Args>
		void error(spdlog::format_string_t<Args...> format, Args&&... args) const;

		template<typename... Args>
		void critical(spdlog::format_string_t<Args...> format, Args&&... args) const;
	};
}

#include "Logger.tpp"
