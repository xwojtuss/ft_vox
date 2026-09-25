#include "Exception.hpp"

namespace error {
	std::string_view domainName(const Domain domain) noexcept {
		switch (domain) {
			case Domain::Filesystem:
				return "Filesystem";
			case Domain::Asset:
				return "Asset";
			case Domain::Ecs:
				return "ECS";
			case Domain::Input:
				return "Input";
			case Domain::Window:
				return "Window";
			case Domain::Render:
				return "Render";
		}
		return "Unknown";
	}

	Exception::Exception(const Domain domain, const std::string& message) : std::runtime_error(message),
																			m_domain(domain) {
	}

	Domain Exception::domain() const noexcept {
		return m_domain;
	}

	std::string_view Exception::domainName() const noexcept {
		return error::domainName(m_domain);
	}

	FileError::FileError(const std::string& path, const std::string& reason) : Exception(Domain::Filesystem,
																					path + ": " + reason),
																				m_path(path) {
	}

	const std::string& FileError::path() const noexcept {
		return m_path;
	}

	AssetError::AssetError(const std::string& path, const std::string& reason) : Exception(Domain::Asset,
			path + ": " + reason), m_path(path) {
	}

	const std::string& AssetError::path() const noexcept {
		return m_path;
	}

	EcsError::EcsError(const std::string& message) : Exception(Domain::Ecs, message) {
	}

	InputError::InputError(const std::string& message) : Exception(Domain::Input, message) {
	}

	WindowError::WindowError(const std::string& message) : Exception(Domain::Window, message) {
	}
}
