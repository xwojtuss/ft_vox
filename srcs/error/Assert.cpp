#include "error/Assert.hpp"

#include <cstdio>
#include <cstdlib>
#include <string>

#include "log/Log.hpp"

namespace {
	[[noreturn]] void logAndAbort(const libassert::assertion_info& info) {
		const std::string report = info.to_string(0, libassert::color_scheme::blank);
		std::fputs(report.c_str(), stderr);
		std::fputc('\n', stderr);
		logging::app().critical("{}", report);
		logging::flush();
		std::abort();
	}
}

void error::installAssertionHandler() {
	libassert::set_failure_handler(logAndAbort);
}
