#include "support/Assertions.hpp"

#include "error/Assert.hpp"

namespace {
	[[noreturn]] void throwAssertion(const libassert::assertion_info& info) {
		throw test::AssertionFailed(info.to_string(0, libassert::color_scheme::blank));
	}
}

void test::reportAssertionsAsExceptions() {
	libassert::set_failure_handler(throwAssertion);
}
