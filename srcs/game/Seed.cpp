#include "game/Seed.hpp"

#include <charconv>
#include <memory>
#include <random>
#include <system_error>

using namespace game;

namespace {
	constexpr std::uint64_t fnvOffsetBasis = 14695981039346656037ULL;
	constexpr std::uint64_t fnvPrime       = 1099511628211ULL;

	std::uint64_t hashText(const std::string_view text) {
		std::uint64_t hash = fnvOffsetBasis;
		for (const char character: text) {
			hash ^= static_cast<unsigned char>(character);
			hash *= fnvPrime;
		}
		return hash;
	}
}

Seed::Seed(const std::uint64_t value) : m_value(value) {
}

Seed Seed::fromText(const std::string_view text) {
	std::int64_t number      = 0;
	const char*  begin       = std::to_address(text.begin());
	const char*  end         = std::to_address(text.end());
	const auto [stop, error] = std::from_chars(begin, end, number);

	if (error == std::errc() && stop == end)
		return Seed(static_cast<std::uint64_t>(number));
	return Seed(hashText(text));
}

Seed Seed::random() {
	std::random_device device;
	return Seed((static_cast<std::uint64_t>(device()) << 32U) | device());
}

std::uint64_t Seed::value() const {
	return m_value;
}
