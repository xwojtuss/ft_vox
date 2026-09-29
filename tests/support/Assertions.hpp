#pragma once

#include <stdexcept>
#include <string>

namespace test {
	class AssertionFailed : public std::runtime_error {
	public:
		explicit AssertionFailed(const std::string& report) : std::runtime_error(report) {
		}
	};

	void reportAssertionsAsExceptions();
}
