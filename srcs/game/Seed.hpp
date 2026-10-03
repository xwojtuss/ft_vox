#pragma once

#include <cstdint>
#include <string_view>

namespace game {
	class Seed {
	private:
		std::uint64_t m_value;

	public:
		explicit Seed(std::uint64_t value);

		[[nodiscard]] static Seed fromText(std::string_view text);
		[[nodiscard]] static Seed random();

		[[nodiscard]] std::uint64_t value() const;
		[[nodiscard]] bool          operator==(const Seed&) const = default;
	};
}
