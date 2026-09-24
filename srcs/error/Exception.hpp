#pragma once

#include <stdexcept>
#include <string>
#include <string_view>

namespace error {
	enum class Domain {
		Filesystem,
		Asset,
		Ecs,
		Input,
		Window,
		Render
	};

	[[nodiscard]] std::string_view domainName(Domain domain) noexcept;

	class Exception : public std::runtime_error {
	private:
		Domain m_domain;

	public:
		Exception(Domain domain, const std::string& message);

		[[nodiscard]] Domain           domain() const noexcept;
		[[nodiscard]] std::string_view domainName() const noexcept;
	};

	class FileError : public Exception {
	private:
		std::string m_path;

	public:
		FileError(const std::string& path, const std::string& reason);

		[[nodiscard]] const std::string& path() const noexcept;
	};

	class AssetError : public Exception {
	private:
		std::string m_path;

	public:
		AssetError(const std::string& path, const std::string& reason);

		[[nodiscard]] const std::string& path() const noexcept;
	};

	class EcsError : public Exception {
	public:
		explicit EcsError(const std::string& message);
	};

	class InputError : public Exception {
	public:
		explicit InputError(const std::string& message);
	};
}
