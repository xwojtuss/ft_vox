#include "Logger.hpp"

using namespace logging;

Logger::Logger(std::shared_ptr<spdlog::logger> logger) : m_logger(std::move(logger)) {
}
