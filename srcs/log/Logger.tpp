#pragma once

namespace logging {
	template<typename... Args>
	void Logger::trace(spdlog::format_string_t<Args...> format, Args&&... args) const {
		m_logger->trace(format, std::forward<Args>(args)...);
	}

	template<typename... Args>
	void Logger::debug(spdlog::format_string_t<Args...> format, Args&&... args) const {
		m_logger->debug(format, std::forward<Args>(args)...);
	}

	template<typename... Args>
	void Logger::info(spdlog::format_string_t<Args...> format, Args&&... args) const {
		m_logger->info(format, std::forward<Args>(args)...);
	}

	template<typename... Args>
	void Logger::warn(spdlog::format_string_t<Args...> format, Args&&... args) const {
		m_logger->warn(format, std::forward<Args>(args)...);
	}

	template<typename... Args>
	void Logger::error(spdlog::format_string_t<Args...> format, Args&&... args) const {
		m_logger->error(format, std::forward<Args>(args)...);
	}

	template<typename... Args>
	void Logger::critical(spdlog::format_string_t<Args...> format, Args&&... args) const {
		m_logger->critical(format, std::forward<Args>(args)...);
	}
}
